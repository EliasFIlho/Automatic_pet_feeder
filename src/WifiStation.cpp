#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include "WifiStation.hpp"

#define WIFI_CONNECT_TIMEOUT 60000
#define WIFI_CALLBACK_FLAGS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | NET_EVENT_WIFI_IFACE_STATUS)
#define WIFI_DHCP_CALLBACK_FLAGS (NET_EVENT_IPV4_DHCP_START | NET_EVENT_IPV4_ADDR_ADD)

LOG_MODULE_REGISTER(WIFI);

struct k_sem wifi_connected;
struct k_sem ipv4_connected;

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;


//TODO: Check RSSI

//TODO: Create a scheduled routine to check the Wireless RSSI and put the value inside a queue - this value can also be mapped in this same
// routine to display the signal quality 


// TODO: See how to use this callback inside of the object field (Is that realy necessary/good?)
static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{

    switch (mgmt_event)
    {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        printk("Connected to wifi network!\r\n");
        k_sem_give(&wifi_connected);
        break;
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        // TODO: Implement a reconnect logic
        printk("Disconnected from the wifi network!\r\n");
        break;
    default:
        break;
    }
}

// TODO: See how to use this callback inside of the object field (Is that realy necessary/good?)
static void dhcp4_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event)
    {
    case NET_EVENT_IPV4_DHCP_START:
        printk("IPV4 Client Initied\r\n");
        break;
    case NET_EVENT_IPV4_ADDR_ADD:
        printk("Device got IPV4 address - release lock\r\n");
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

void WifiStation::wifi_init(void)
{
    sta_iface = net_if_get_wifi_sta();
    net_mgmt_init_event_callback(&ipv4_cb, dhcp4_event_handler, WIFI_DHCP_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&ipv4_cb);
    net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler, WIFI_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&wifi_cb);
}

int WifiStation::connect_to_wifi(char *ssid, char *psk)
{

    if (!sta_iface)
    {
        LOG_INF("STA: interface no initialized");
        return -EIO;
    }

    struct wifi_connect_req_params params;
    int ret;

    params.ssid = (const uint8_t *)ssid;
    params.ssid_length = strlen(ssid);
    params.psk = (const uint8_t *)psk;
    params.psk_length = strlen(psk);
    params.security = WIFI_SECURITY_TYPE_PSK;
    params.band = WIFI_FREQ_BAND_UNKNOWN;
    params.channel = WIFI_CHANNEL_ANY;
    params.mfp = WIFI_MFP_OPTIONAL;

    ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &params, sizeof(params));
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

    return ret;
}

int WifiStation::wifi_wait_for_ip_addr(void)
{
    struct wifi_iface_status status;
    if (!sta_iface)
    {
        LOG_INF("STA: interface no initialized");
        return -EIO;
    }
    char ip_addr[NET_IPV4_ADDR_LEN];
    char gateway_addr[NET_IPV4_ADDR_LEN];
    int ret;

    printk("Waiting for IP address");
    k_sem_take(&ipv4_connected, K_FOREVER);

    ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, sta_iface, &status, sizeof(struct wifi_iface_status));
    if (ret)
    {
        printk("Error to request Wifi status\r\n");
    }
    else
    {
        memset(ip_addr, 0, sizeof(ip_addr));
        if (net_addr_ntop(AF_INET, &sta_iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr, ip_addr, sizeof(ip_addr)) == NULL)
        {
            printk("Error to convert IP addr to string");
        }

        memset(gateway_addr, 0, sizeof(gateway_addr));
        if (net_addr_ntop(AF_INET, &sta_iface->config.ip.ipv4->gw, gateway_addr, sizeof(gateway_addr)) == NULL)
        {
            printk("Error to convert Gateway IP addr to string");
        }
    }

    printk("Wifi status:\r\n");

    if (status.state >= WIFI_STATE_ASSOCIATED)
    {
        printk(" SSID: %-32s\r\n", status.ssid);
        printk(" BAND: %s\r\n", wifi_band_txt(status.band));
        printk(" CHANNEL: %d\r\n", status.channel);
        printk(" SECURITY: %s\r\n", wifi_security_txt(status.security));
        printk(" IP Addr: %s\r\n", ip_addr);
        printk(" Gateway Addr: %s\r\n", gateway_addr);
    }
    return 0;
}

int WifiStation::wait_wifi_to_connect(void)
{
    printk("Waiting for wifi connection signal\n\r");
    if (k_sem_take(&wifi_connected, K_MSEC(WIFI_CONNECT_TIMEOUT)) != 0)
    {
        LOG_ERR("UNABLE TO CONNECT TO WIFI\r\n");
        return -1;
    }
    else
    {
        LOG_INF("CONNECTED TO WIFI NETWOKR\r\n");
        return 0;
    }
}

int WifiStation::wifi_disconnect(void)
{
    int ret;

    ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, sta_iface, NULL, 0);
    return ret;
}

// TODO: Create object atributes and assign these parameters into
void WifiStation::set_wifi_credentials(const char *ssid, const char *psk)
{
}
