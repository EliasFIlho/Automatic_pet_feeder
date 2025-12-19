#include <string.h>
#include "WifiStation.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(NETWORK_LOGS);

#define WIFI_CALLBACK_FLAGS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | NET_EVENT_WIFI_IFACE_STATUS | NET_EVENT_WIFI_SCAN_DONE)
#define WIFI_DHCP_CALLBACK_FLAGS (NET_EVENT_IPV4_DHCP_START | NET_EVENT_IPV4_ADDR_ADD)

void WifiStation::timeout_tmr_handler(struct k_timer *timer_id)
{
    WifiStation &instance = WifiStation::Get_Instance();
    instance.notify_evt(Events::TIMEOUT);
}

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
            k_timer_stop(&instance.TIMEOUT_TMR);
            LOG_INF("WIFI Connected");
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
        k_timer_stop(&instance.TIMEOUT_TMR);
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
    k_timer_init(&this->TIMEOUT_TMR, this->timeout_tmr_handler, NULL);
    this->sta_iface = net_if_get_wifi_sta();
    if (sta_iface == NULL)
    {
        this->notify_evt(Events::WIFI_IFACE_ERROR);
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
        this->notify_evt(Events::WIFI_IFACE_ERROR);
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

void WifiStation::stop_dhcp()
{
    net_dhcpv4_stop(this->sta_iface);
}

int WifiStation::wifi_wait_for_ip_addr(void)
{

    k_timer_start(&this->TIMEOUT_TMR, K_SECONDS(CONFIG_WIFI_GET_IP_TIMEOUT), K_NO_WAIT);
    return 0;
}

int WifiStation::wait_wifi_to_connect(void)
{
    k_timer_start(&this->TIMEOUT_TMR, K_SECONDS(CONFIG_WIFI_CONNECT_TIMEOUT), K_NO_WAIT);
    return 0;
}

int WifiStation::wifi_disconnect(void)
{
    int ret;

    ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, sta_iface, NULL, 0);

    return ret;
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


//TODO: Get a filesystem referencia herer and use the return of the methods to notify events
void WifiStation::set_credentials(const char *ssid, const char *psk)
{
    strcpy(this->ssid, ssid);
    strcpy(this->psk, psk);
    notify_evt(Events::WIFI_CREDS_OK);
}