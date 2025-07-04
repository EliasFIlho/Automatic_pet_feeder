#include "WifiSoftAp.hpp"

#define WIFI_AP_SSID "ESP32-AP"
#define WIFI_AP_PSK "12345678"

#define WIFI_AP_BASE_IP4_ADDR "192.168.4.1"
#define WIFI_AP_NETMASK    "255.255.255.0"

#define AP_EVENT_MASK (NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT)

static struct net_mgmt_event_callback ap_cb;

static void sap_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event)
    {

    case NET_EVENT_WIFI_AP_ENABLE_RESULT:
        printk("AP Mode is enabled. Waiting for station to connect\r\n");
        break;

    case NET_EVENT_WIFI_AP_DISABLE_RESULT:
        printk("AP Mode is disabled.\r\n");
        break;

    default:
        printk("Any other event - [%d]\r\n", mgmt_event);
        break;
    }
}

WifiSoftAp::WifiSoftAp()
{
    this->sap_iface = net_if_get_wifi_sap();
    net_mgmt_init_event_callback(&ap_cb, sap_event_handler, AP_EVENT_MASK);
    net_mgmt_add_event_callback(&ap_cb);
}

WifiSoftAp::~WifiSoftAp()
{
}

int WifiSoftAp::enable_dhcpv4_server()
{
    struct in_addr base_addr;
    struct in_addr net_mask;

    int ret = net_addr_pton(AF_INET, WIFI_AP_BASE_IP4_ADDR, &base_addr);
    if (ret != 0)
    {
        printk("Invalid IP addr\r\n");
        return ret;
    }

    ret = net_addr_pton(AF_INET, WIFI_AP_NETMASK, &net_mask);

    if (ret != 0)
    {
        printk("Invalid MASK IP addr\r\n");
        return ret;
    }


    // Loop-back??
    net_if_ipv4_set_gw(this->sap_iface, &base_addr);

    if (net_if_ipv4_addr_add(this->sap_iface, &base_addr, NET_ADDR_MANUAL, 0) == NULL)
    {
        printk("Unable to set IP address for AP interface\r\n");
        return -1;
    }

    if (!net_if_ipv4_set_netmask_by_addr(this->sap_iface, &base_addr, &net_mask)) {
		printk("Unable to set netmask for AP interface: %s", WIFI_AP_NETMASK);
        return -1;
	}

    base_addr.s4_addr[3] += 10;
    ret = net_dhcpv4_server_start(this->sap_iface, &base_addr);
    return ret;
}

void WifiSoftAp::populate_ap_configs()
{

    this->ap_config.ssid = (const uint8_t *)WIFI_AP_SSID;
    this->ap_config.ssid_length = strlen(WIFI_AP_SSID);
    this->ap_config.psk = (const uint8_t *)WIFI_AP_PSK;
    this->ap_config.psk_length = strlen(WIFI_AP_PSK);
    this->ap_config.channel = WIFI_CHANNEL_ANY;
    this->ap_config.band = WIFI_FREQ_BAND_2_4_GHZ;
    this->ap_config.security = WIFI_SECURITY_TYPE_PSK;
}

int WifiSoftAp::enable_soft_ap()
{

    // Config soft ap parameters
    // Enable dhcp4 server
    // Return a code

    if (!this->sap_iface)
    {
        printk("AP: is not initialized");
        return -EIO;
    }

    this->populate_ap_configs();

    int ret = this->enable_dhcpv4_server();
    if (ret != 0)
    {
        printk("Error to enable DHCPV4 Server\r\n");
        return -1;
    }

    ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, this->sap_iface, &this->ap_config,
                   sizeof(struct wifi_connect_req_params));
    if (ret)
    {
        printk("NET_REQUEST_WIFI_AP_ENABLE failed, err: %d", ret);
    }

    return ret;
}