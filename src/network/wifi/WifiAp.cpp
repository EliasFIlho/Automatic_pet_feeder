#include "WifiAp.hpp"

LOG_MODULE_DECLARE(NETWORK_LOGS);

#define NET_EVENT_WIFI_MASK (NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT | \
                             NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

WifiAp::WifiAp(/* args */)
{
}

WifiAp::~WifiAp()
{
}
void WifiAp::wifi_ap_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    auto *self = CONTAINER_OF(cb, WifiAp, ap_cb);

    switch (mgmt_event)
    {

    case NET_EVENT_WIFI_AP_ENABLE_RESULT:
    {
        LOG_INF("AP Mode is enabled. Waiting for station to connect");
        self->isSoftAPEnabled = true;
        self->notify_evt(Events::WIFI_AP_ENABLE);
        LOG_INF("AP Inited");
        break;
    }
    case NET_EVENT_WIFI_AP_DISABLE_RESULT:
    {
        LOG_INF("AP Mode is disabled.");
        self->isSoftAPEnabled = false;
        self->notify_evt(Events::WIFI_AP_DISABLED);
        break;
    }
    case NET_EVENT_WIFI_AP_STA_CONNECTED:
    {
        struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

        LOG_INF("station: joined ");
        break;
    }
    case NET_EVENT_WIFI_AP_STA_DISCONNECTED:
    {
        struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

        LOG_INF("station: leave ");
        break;
    }
    default:
        break;
    }
}

void WifiAp::config_dhcp_server()
{
    if (net_addr_pton(AF_INET, CONFIG_WIFI_AP_IP, &this->addr) < 0)
    {
        LOG_ERR("Fail to convert ip address");
    }
    if (net_addr_pton(AF_INET, CONFIG_WIFI_AP_MASK, &this->netmaskAddr) < 0)
    {
        LOG_ERR("Fail to convert mask address");
    }

    net_if_ipv4_set_gw(this->ap_iface, &this->addr);

    if (net_if_ipv4_addr_add(this->ap_iface, &this->addr, NET_ADDR_MANUAL, 0) == NULL)
    {
        LOG_ERR("unable to set IP address for AP interface");
    }

    if (!net_if_ipv4_set_netmask_by_addr(this->ap_iface, &this->addr, &this->netmaskAddr))
    {
        LOG_ERR("Unable to set netmask for AP interface:");
    }

    addr.s4_addr[3] += 10;

    if (net_dhcpv4_server_start(this->ap_iface, &this->addr) != 0)
    {
        LOG_ERR("DHCP server is not started for desired IP");
        return;
    }
}

void WifiAp::ap_init(void)
{
    net_mgmt_init_event_callback(&this->ap_cb, this->wifi_ap_event_handler, NET_EVENT_WIFI_MASK);
    net_mgmt_add_event_callback(&this->ap_cb);
    this->ap_iface = net_if_get_wifi_sap();
    if (ap_iface == NULL)
    {
        LOG_ERR("ERROR TO GET AP IFACE");
        this->notify_evt(Events::WIFI_IFACE_ERROR);
        return;
    }

    memset(&this->ap_config, 0, sizeof(this->ap_config));
    this->ap_config.ssid = (const uint8_t *)CONFIG_WIFI_AP_SSID;
    this->ap_config.ssid_length = sizeof(CONFIG_WIFI_AP_SSID) - 1;
    this->ap_config.psk = (const uint8_t *)CONFIG_WIFI_AP_PSK;
    this->ap_config.psk_length = sizeof(CONFIG_WIFI_AP_PSK) - 1;
    this->ap_config.channel = WIFI_CHANNEL_ANY;
    this->ap_config.band = WIFI_FREQ_BAND_2_4_GHZ;
    this->ap_config.security = WIFI_SECURITY_TYPE_PSK;

    this->config_dhcp_server();

    int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, this->ap_iface, &this->ap_config, sizeof(struct wifi_connect_req_params));
    if (ret != 0)
    {
        this->notify_evt(Events::WIFI_IFACE_ERROR); // Create error to return
        LOG_ERR("ERROR TO REQUEST WIFI AP START - %d", ret);
    }
}
void WifiAp::ap_start(void)
{
}
void WifiAp::ap_stop(void)
{
    net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, this->ap_iface, NULL, NULL);
}

void WifiAp::notify_evt(Events evt)
{
    EventMsg msg{.evt = evt,
                 .type = EventGroup::WIFI};

    k_msgq_put(&net_evt_queue, &msg, K_NO_WAIT);
}

bool WifiAp::is_ap_enabled()
{
    return this->isSoftAPEnabled;
}