#include <string.h>
#include "WifiStation.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(NETWORK_LOGS);

#define WIFI_CALLBACK_FLAGS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | NET_EVENT_WIFI_IFACE_STATUS | NET_EVENT_WIFI_SCAN_DONE)
#define WIFI_DHCP_CALLBACK_FLAGS (NET_EVENT_IPV4_DHCP_START | NET_EVENT_IPV4_ADDR_ADD)

// TODO: Move all this to class header
static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static struct k_sem wifi_connected;
static struct k_sem ipv4_connected;

void WifiStation::wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t evt, struct net_if *iface)
{
    WifiStation &instance = WifiStation::Get_Instance();

    switch (evt)
    {
    case NET_EVENT_WIFI_CONNECT_RESULT:
    {
        const struct wifi_status *st = (const struct wifi_status *)cb->info;
        int status = st ? st->status : -1;
        if (status == 0)
        {
            LOG_INF("WIFI Connected");
            k_sem_give(&wifi_connected);
            instance.con_state = CON_STATE::CONNECTED;
        }

        break;
    }
    case NET_EVENT_WIFI_DISCONNECT_RESULT:

        LOG_WRN("DEVICE DISCONNECTD FROM NETWORK");
        instance.on_disconnect();
        break;

    case NET_EVENT_WIFI_IFACE_STATUS:
        break;
    case NET_EVENT_WIFI_SCAN_DONE:
        LOG_WRN("SCAN DONE");
    default:
        LOG_WRN("UNEXPECTED EVENT - %d",evt);
        break;
    }
}

// TODO: Move this to class
static void dhcp4_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event)
    {
    case NET_EVENT_IPV4_DHCP_START:
        LOG_INF("DHCP4 Started");
        break;
    case NET_EVENT_IPV4_ADDR_ADD:
        LOG_INF("Device got IP");
        k_sem_give(&ipv4_connected);
        break;
    default:
        break;
    }
}

WifiStation::WifiStation()
{
    k_sem_init(&wifi_connected, 0, 1);
    k_sem_init(&ipv4_connected, 0, 1);
}

WifiStation::~WifiStation()
{
}

bool WifiStation::wifi_init(void)
{
    k_sem_reset(&wifi_connected);
    k_sem_reset(&ipv4_connected);
    this->sta_iface = net_if_get_wifi_sta();
    if (sta_iface == NULL)
    {
        return false;
    }
    net_mgmt_init_event_callback(&ipv4_cb, dhcp4_event_handler, WIFI_DHCP_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&ipv4_cb);
    net_mgmt_init_event_callback(&wifi_cb, this->wifi_event_handler, WIFI_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&wifi_cb);
    k_work_init_delayable(&this->reconnect_k_work, this->reconnect_work);
    int ret = net_if_up(sta_iface);
    if (ret && ret != -EALREADY && ret != -ENOTSUP)
    {
        return false;
    }
    else
    {
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

    struct wifi_connect_req_params params = {0}; // Initializ as 0 to avoid garbage
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
    ret = wait_wifi_to_connect();
    if (ret < 0)
    {
        return ret;
    }

    ret = wifi_wait_for_ip_addr();
    if (ret < 0)
    {
        return ret;
    }
    LOG_INF("No error in net_mgmt: %d", ret);
    return ret;
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
        return -1;
    }
    else
    {
        LOG_ERR("CONNECTED TO WIFI NETWORK");
        this->con_state = CON_STATE::CONNECTED;
        return 0;
    }
}

int WifiStation::wifi_disconnect(void)
{
    int ret;

    ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, sta_iface, NULL, 0);
    this->con_state = CON_STATE::DISCONNECTED;
    return ret;
}

void WifiStation::set_wifi_ssid(char *ssid)
{
    strcpy(this->ssid, ssid);
}
void WifiStation::set_wifi_psk(char *psk)
{
    strcpy(this->psk, psk);
}

bool WifiStation::is_connected()
{
    struct wifi_iface_status status;

    net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, this->sta_iface, &status, sizeof(struct wifi_iface_status));
    if (status.state == WIFI_STATE_DISCONNECTED)
    {
        LOG_WRN("WIFI DISCONNECTD");

        return false;
    }
    else
    {
        LOG_INF("WIFI CONNECTED - WIFI RSSI: %d", status.rssi);
        return true;
    }
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
void WifiStation::on_disconnect()
{

#if !CONFIG_ESP32_WIFI_STA_RECONNECT
        k_work_reschedule(&this->reconnect_k_work, K_SECONDS(30));
#endif

}

void WifiStation::reconnect_work(struct k_work *work)
{
    WifiStation &instance = WifiStation::Get_Instance();
    int ret = instance.connect_to_wifi();
    if (ret < 0)
    {
        k_work_delayable *dwork = k_work_delayable_from_work(work);
        k_work_reschedule(dwork, K_SECONDS(30));
    }
}

WifiStation &WifiStation::Get_Instance()
{
    static WifiStation inst;
    return inst;
}