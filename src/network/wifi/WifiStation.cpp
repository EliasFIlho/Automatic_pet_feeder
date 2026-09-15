#include <string.h>
#include "WifiStation.hpp"
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_credentials.h>

LOG_MODULE_DECLARE(NETWORK_LOGS);

#define WIFI_CALLBACK_FLAGS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | NET_EVENT_WIFI_IFACE_STATUS | NET_EVENT_WIFI_SCAN_DONE | NET_EVENT_WIFI_SCAN_RESULT)
#define WIFI_DHCP_CALLBACK_FLAGS (NET_EVENT_IPV4_DHCP_START | NET_EVENT_IPV4_ADDR_ADD)

#define SSID_TEMP_BUFFER_LEN 33
#define PSK_TEMP_BUFFER_LEN 64

void WifiStation::timeout_tmr_handler(struct k_timer *timer_id)
{
    auto *self = static_cast<WifiStation *>(k_timer_user_data_get(timer_id));

    self->notify_evt(Events::TIMEOUT, NULL);
}

void WifiStation::wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    auto *self = CONTAINER_OF(cb, WifiStation, wifi_cb);

    switch (mgmt_event)
    {
    case NET_EVENT_WIFI_CONNECT_RESULT:
    {
        const struct wifi_status *st = (const struct wifi_status *)cb->info;
        int status = st ? st->status : -1;
        if (status == 0)
        {
            self->stop_connect_timer();
            LOG_INF("WIFI Connected");
            self->notify_evt(Events::WIFI_CONNECTED, NULL);
        }

        break;
    }
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
    {
        const struct wifi_status *st = (const struct wifi_status *)cb->info;
        LOG_WRN("DEVICE DISCONNECTD FROM NETWORK - REASON: %d", st->disconn_reason);
        self->notify_evt(Events::WIFI_DISCONNECTED, NULL);
        break;
    }
    case NET_EVENT_WIFI_IFACE_STATUS:
        break;
    case NET_EVENT_WIFI_SCAN_DONE:
        LOG_WRN("SCAN DONE");
        break;

    case NET_EVENT_WIFI_SCAN_RESULT:
    {
        const struct wifi_scan_result *scan_result = (struct wifi_scan_result *)cb->info;
        LOG_WRN("SCAN RESULT: %s", scan_result->ssid);
        break;
    }
    default:
        LOG_WRN("UNEXPECTED EVENT - %lld", mgmt_event);
        break;
    }
}

void WifiStation::dhcp4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{

    auto *self = CONTAINER_OF(cb, WifiStation, ipv4_cb);
    switch (mgmt_event)
    {
    case NET_EVENT_IPV4_DHCP_START:
        LOG_INF("DHCP4 Started");
        break;
    case NET_EVENT_IPV4_ADDR_ADD:
    {

        LOG_INF("Device got IP");
        self->stop_connect_timer();
        self->notify_evt(Events::WIFI_IP_ACQUIRED, NULL);
        break;
    }
    default:
        break;
    }
}

WifiStation::WifiStation(IStorage &fs) : _fs(fs)
{
}

WifiStation::~WifiStation()
{
}

bool WifiStation::wifi_init(void)
{
    k_timer_init(&this->TIMEOUT_TMR, this->timeout_tmr_handler, NULL);
    k_timer_user_data_set(&this->TIMEOUT_TMR, this);
    this->sta_iface = net_if_get_wifi_sta();
    if (sta_iface == NULL)
    {
        this->notify_evt(Events::WIFI_IFACE_ERROR, NULL);
        return false;
    }
    net_mgmt_init_event_callback(&this->ipv4_cb, dhcp4_event_handler, WIFI_DHCP_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&this->ipv4_cb);
    net_mgmt_init_event_callback(&this->wifi_cb, this->wifi_event_handler, WIFI_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&this->wifi_cb);
    int ret = net_if_up(sta_iface);
    if (ret && ret != -EALREADY && ret != -ENOTSUP)
    {
        return false;
    }
    else
    {
        this->notify_evt(Events::WIFI_IFACE_UP, NULL);
        return true;
    }
}

void WifiStation::scan_ssid_list()
{
    net_mgmt(NET_REQUEST_WIFI_SCAN, this->sta_iface, this, sizeof(this));
}

void WifiStation::connect_to_wifi()
{
    if (!this->sta_iface)
    {
        LOG_ERR("STA: interface no initialized");
        this->notify_evt(Events::WIFI_IFACE_ERROR, NULL);
        return;
    }
#if !CONFIG_WIFI_CREDENTIALS

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
        this->notify_evt(Events::WIFI_IFACE_ERROR, NULL);
        return;
    }
    LOG_INF("REQUEST WIFI CONNECT RESULT: %d", ret);
#else
    this->connect_to_wifi_stored();

#endif

    wait_wifi_to_connect();
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

void WifiStation::wait_wifi_to_connect(void)
{
    k_timer_start(&this->TIMEOUT_TMR, K_SECONDS(CONFIG_WIFI_CONNECT_TIMEOUT), K_NO_WAIT);
}

int WifiStation::wifi_disconnect(void)
{
    int ret;

    ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, sta_iface, NULL, 0);
    LOG_INF("WIFI DISCONNECT REQUESTED: %d", ret);
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

void WifiStation::notify_evt(Events evt, void *data)
{
    EventMsg msg{.evt = evt,
                 .type = WIFI_EVT};

    k_msgq_put(&net_evt_queue, &msg, K_NO_WAIT);
}

void WifiStation::set_credentials()
{
#if !CONFIG_WIFI_CREDENTIALS
    char ssid[SSID_TEMP_BUFFER_LEN];
    char psk[PSK_TEMP_BUFFER_LEN];
    if (this->_fs.read_buffer(SSID_ID, ssid, sizeof(ssid)) < 0)
    {
        this->notify_evt(Events::WIFI_CREDS_NOT_FOUND, NULL);
        return;
    }
    if (this->_fs.read_buffer(PASSWORD_ID, psk, sizeof(psk)) < 0)
    {
        this->notify_evt(Events::WIFI_CREDS_NOT_FOUND, NULL);
        return;
    }

    LOG_INF("SSID: '%s' (len=%d)", ssid, strlen(ssid));
    LOG_INF("PSK length: %d", strlen(psk));
    strcpy(this->ssid, ssid);
    strcpy(this->psk, psk);
    notify_evt(Events::WIFI_CREDS_OK, NULL);
#else

    if (wifi_credentials_is_empty())
    {
        this->notify_evt(Events::WIFI_CREDS_NOT_FOUND, NULL);
        return;
    }
    else
    {
        notify_evt(Events::WIFI_CREDS_OK, NULL);
        return;
    }
#endif
}

void WifiStation::stop_connect_timer()
{
    k_timer_stop(&this->TIMEOUT_TMR);
}

int WifiStation::connect_to_wifi_stored()
{
    if (!this->sta_iface)
    {
        LOG_ERR("STA: interface no initialized");
        this->notify_evt(Events::WIFI_IFACE_ERROR, NULL);
        return -1;
    }

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, this->sta_iface, NULL, 0);
    if (ret != 0)
    {
        LOG_ERR("Error in net_mgmt: %d", ret);
        this->notify_evt(Events::WIFI_IFACE_ERROR, NULL);
        return -1;
    }
    LOG_INF("REQUEST WIFI CONNECT RESULT: %d", ret);
    return 0;
}

