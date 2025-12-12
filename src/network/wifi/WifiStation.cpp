#include <string.h>
#include "WifiStation.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(NETWORK_LOGS);

#define WIFI_CALLBACK_FLAGS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | NET_EVENT_WIFI_IFACE_STATUS | NET_EVENT_WIFI_SCAN_DONE)
#define WIFI_DHCP_CALLBACK_FLAGS (NET_EVENT_IPV4_DHCP_START | NET_EVENT_IPV4_ADDR_ADD)

void WifiStation::wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    WifiStation &instance = WifiStation::Get_Instance();

    switch (mgmt_event)
    {
    case NET_EVENT_WIFI_CONNECT_RESULT:
    {
        const struct wifi_status *st = (const struct wifi_status *)cb->info;
        int status = st ? st->status : -1;
        if (status == 0)
        {
            LOG_INF("WIFI Connected");
            k_sem_give(&instance.wifi_connected);
            instance.notify_evt(Events::WIFI_CONNECTED);
        }

        break;
    }
    case NET_EVENT_WIFI_DISCONNECT_RESULT:

        LOG_WRN("DEVICE DISCONNECTD FROM NETWORK");
        instance.notify_evt(Events::WIFI_DISCONNECTED);
        break;

    case NET_EVENT_WIFI_IFACE_STATUS:
        break;
    case NET_EVENT_WIFI_SCAN_DONE:
        LOG_WRN("SCAN DONE");
        break;
    default:
        LOG_WRN("UNEXPECTED EVENT - %lld", mgmt_event);
        break;
    }
}

void WifiStation::dhcp4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    WifiStation &instance = WifiStation::Get_Instance();

    switch (mgmt_event)
    {
    case NET_EVENT_IPV4_DHCP_START:
        LOG_INF("DHCP4 Started");
        break;
    case NET_EVENT_IPV4_ADDR_ADD:
    {

        LOG_INF("Device got IP");
        
        k_sem_give(&instance.ipv4_connected);
        LOG_INF("Ip Semaphore released, throwing ip event");
        instance.notify_evt(Events::IP_ACQUIRED);
        break;
    }
    default:
        break;
    }
}

WifiStation::WifiStation()
{
}

WifiStation::~WifiStation()
{
}

bool WifiStation::wifi_init(void)
{
    k_sem_init(&this->wifi_connected, 0, 1);
    k_sem_init(&this->ipv4_connected, 0, 1);
    k_sem_reset(&this->wifi_connected);
    k_sem_reset(&this->ipv4_connected);
    this->sta_iface = net_if_get_wifi_sta();
    if (sta_iface == NULL)
    {
        return false;
    }
    net_mgmt_init_event_callback(&this->ipv4_cb, dhcp4_event_handler, WIFI_DHCP_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&this->ipv4_cb);
    net_mgmt_init_event_callback(&this->wifi_cb, this->wifi_event_handler, WIFI_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&this->wifi_cb);
    // Perform NET_EVENT_WIFI_IFACE_STATUS req
    int ret = net_if_up(sta_iface);
    if (ret && ret != -EALREADY && ret != -ENOTSUP)
    {
        this->notify_evt(Events::WIFI_IFACE_ERROR);
        return false;
    }
    else
    {
        this->notify_evt(Events::WIFI_IFACE_UP);
        return true;
    }
}

int WifiStation::connect_to_wifi()
{
    if (!this->sta_iface)
    {
        LOG_ERR("STA: interface no initialized");
        return -EIO;
    }

    struct wifi_connect_req_params params = {0}; // Initialize as 0 to avoid garbage
    int ret;

    params.ssid = (const uint8_t *)this->ssid;
    params.ssid_length = strlen(this->ssid);
    params.psk = (const uint8_t *)this->psk;
    params.psk_length = strlen(this->psk);
    params.security = WIFI_SECURITY_TYPE_PSK;
    params.band = WIFI_FREQ_BAND_UNKNOWN;
    params.channel = WIFI_CHANNEL_ANY;
    params.mfp = WIFI_MFP_OPTIONAL;

    ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, this->sta_iface, &params, sizeof(params));
    if (ret != 0)
    {
        LOG_ERR("Error in net_mgmt: %d", ret);
        return ret;
    }
    else
    {
        LOG_WRN("NET_MGMT RETURN %d", ret);
    }

    ret = wait_wifi_to_connect();
    if (ret < 0)
    {
        return ret;
    }
}

void WifiStation::start_dhcp()
{
    net_dhcpv4_start(this->sta_iface);
    wifi_wait_for_ip_addr();
}

int WifiStation::wifi_wait_for_ip_addr(void)
{
    struct wifi_iface_status status;
    if (!sta_iface)
    {
        LOG_ERR("STA: interface no initialized");
        return -EIO;
    }
    char ip_addr[NET_IPV4_ADDR_LEN];
    char gateway_addr[NET_IPV4_ADDR_LEN];
    int ret;

    if (k_sem_take(&ipv4_connected, K_SECONDS(CONFIG_WIFI_GET_IP_TIMEOUT)) != 0)
    {
        LOG_ERR("UNABLE TO GET IP ADDRESS - TIMEOUT");
        this->notify_evt(Events::TIMEOUT);
        return -1;
    }

    ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, sta_iface, &status, sizeof(struct wifi_iface_status));
    if (ret)
    {
        LOG_ERR("Error to request Wifi status\r\n");
    }
    else
    {
        memset(ip_addr, 0, sizeof(ip_addr));
        if (net_addr_ntop(AF_INET, &sta_iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr, ip_addr, sizeof(ip_addr)) == NULL)
        {
            LOG_ERR("Error to convert IP addr to string");
        }

        memset(gateway_addr, 0, sizeof(gateway_addr));
        if (net_addr_ntop(AF_INET, &sta_iface->config.ip.ipv4->gw, gateway_addr, sizeof(gateway_addr)) == NULL)
        {
            LOG_ERR("Error to convert Gateway IP addr to string");
        }
    }

    if (status.state >= WIFI_STATE_ASSOCIATED)
    {
        LOG_INF(" SSID: %-32s", status.ssid);
        LOG_INF(" BAND: %s", wifi_band_txt(status.band));
        LOG_INF(" CHANNEL: %d", status.channel);
        LOG_INF(" SECURITY: %s", wifi_security_txt(status.security));
        LOG_INF(" IP Addr: %s", ip_addr);
        LOG_INF(" Gateway Addr: %s", gateway_addr);
    }
    return 0;
}

int WifiStation::wait_wifi_to_connect(void)
{
    LOG_WRN("Waiting for wifi connection signal");
    if (k_sem_take(&wifi_connected, K_SECONDS(CONFIG_WIFI_CONNECT_TIMEOUT)) != 0)
    {
        LOG_ERR("UNABLE TO CONNECT TO WIFI - TIMEOUT");
        this->notify_evt(Events::TIMEOUT);
        return -1;
    }
    else
    {
        LOG_INF("CONNECTED TO WIFI NETWORK");

        return 0;
    }
}

int WifiStation::wifi_disconnect(void)
{
    int ret;

    ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, sta_iface, NULL, 0);

    return ret;
}

bool WifiStation::is_connected()
{
    return false;
}

int32_t WifiStation::get_rssi()
{
    if (this->sta_iface != NULL)
    {
        struct wifi_iface_status status;
        net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, this->sta_iface, &status, sizeof(struct wifi_iface_status));
        return status.rssi;
    }
    else
    {
        return 0;
    }
}

WifiStation &WifiStation::Get_Instance()
{
    static WifiStation inst;
    return inst;
}

void WifiStation::notify_evt(Events evt)
{
    EventMsg msg{.evt = evt,
                 .type = EventGroup::WIFI};

    k_msgq_put(&net_evt_queue, &msg, K_NO_WAIT);
}

void WifiStation::set_credentials(const char *ssid, const char *psk)
{
    strcpy(this->ssid, ssid);
    strcpy(this->psk, psk);

    notify_evt(Events::WIFI_CREDS_OK);
}