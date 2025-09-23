#include <string.h>
#include "WifiStation.hpp"

#define WIFI_CONNECT_TIMEOUT 30000

#define WIFI_CALLBACK_FLAGS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | NET_EVENT_WIFI_IFACE_STATUS)
#define WIFI_DHCP_CALLBACK_FLAGS (NET_EVENT_IPV4_DHCP_START | NET_EVENT_IPV4_ADDR_ADD)

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;
static struct net_mgmt_event_callback if_cb;

static struct k_sem wifi_connected;
static struct k_sem ipv4_connected;

// TODO: Check RSSI with wifi_iface_status.rssi
// TODO: Create a scheduled routine to check the Wireless RSSI and put the value inside a queue - this value can also be mapped in this same
//  routine to display the signal quality - Maybe use Work Queue to do it

static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint32_t evt, struct net_if *iface)
{
    if (evt == NET_EVENT_WIFI_CONNECT_RESULT)
    {
        const struct wifi_status *st = (const struct wifi_status *)cb->info;
        int status = st ? st->status : -1;
        printk("WiFi CONNECT result=%d\n", status);
        if (status == 0)
        {
            k_sem_give(&wifi_connected);
        }
    }
    else if (evt == NET_EVENT_WIFI_DISCONNECT_RESULT)
    {
        const struct wifi_status *st = (const struct wifi_status *)cb->info;
        printk("WiFi DISCONNECT status=%d\n", st ? st->status : -1);
    }
    else if (evt = NET_EVENT_WIFI_IFACE_STATUS)
    {
        printk("WIFI IF STATUS EVT\n\r");
    }
}

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

bool WifiStation::wifi_init(void)
{
    k_sem_reset(&wifi_connected);
    k_sem_reset(&ipv4_connected);
    sta_iface = net_if_get_wifi_sta();
    net_mgmt_init_event_callback(&ipv4_cb, dhcp4_event_handler, WIFI_DHCP_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&ipv4_cb);
    net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler, WIFI_CALLBACK_FLAGS);
    net_mgmt_add_event_callback(&wifi_cb);
    int ret = net_if_up(sta_iface);
    if (ret && ret != -EALREADY && ret != -ENOTSUP)
    {
        printk("net_if_up failed: %d\n", ret);
        return false;
    }
    else
    {
        printk("Interface up (or already up / not required)\n");
        return true;
    }
}

int WifiStation::connect_to_wifi()
{

    if (!sta_iface)
    {
        printk("STA: interface no initialized");
        return -EIO;
    }

    struct wifi_connect_req_params params;
    int ret;

    params.ssid = (const uint8_t *)this->ssid;
    params.ssid_length = strlen(this->ssid);
    params.psk = (const uint8_t *)this->psk;
    params.psk_length = strlen(this->psk);
    params.security = WIFI_SECURITY_TYPE_PSK;
    params.band = WIFI_FREQ_BAND_UNKNOWN;
    params.channel = WIFI_CHANNEL_ANY;
    params.mfp = WIFI_MFP_OPTIONAL;

    printk("Connecting SSID='%s' len=%zu, PSK len=%zu\n", this->ssid, strlen(this->ssid), strlen(this->psk));

    ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &params, sizeof(params));
    printk("net_mgmt CONNECT ret=%d\n", ret);
    if (ret != 0)
    {
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

    return ret;
}

int WifiStation::wifi_wait_for_ip_addr(void)
{
    struct wifi_iface_status status;
    if (!sta_iface)
    {
        printk("STA: interface no initialized");
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
        printk("UNABLE TO CONNECT TO WIFI\r\nTIMEOUT\n\r");
        return -1;
    }
    else
    {
        printk("CONNECTED TO WIFI NETWORK\r\n");
        return 0;
    }
}

int WifiStation::wifi_disconnect(void)
{
    int ret;

    ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, sta_iface, NULL, 0);
    return ret;
}

void WifiStation::set_wifi_ssid(char *ssid)
{
    strcpy(this->ssid, ssid);
    // printk("%s\n\r", this->ssid);
}
void WifiStation::set_wifi_psk(char *psk)
{
    strcpy(this->psk, psk);
    // printk("%s\n\r", this->psk);
}

bool WifiStation::is_connected()
{
    struct wifi_iface_status status;

    int ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, sta_iface, &status, sizeof(struct wifi_iface_status));
    printk("IS CONNECT NET REQUEST RETURN: %d\n\r", ret);
    if (status.state == WIFI_STATE_DISCONNECTED)
    {
        printk("WIFI DISCONNECTD\n\r");

        return false;
    }
    else
    {
        printk("WIFI CONNECTED\n\rWIFI RSSI: %d\n\r", status.rssi);
        return true;
    }
}

int WifiStation::wifi_get_rssi()
{
    struct wifi_iface_status status;
    int ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, sta_iface, &status, sizeof(struct wifi_iface_status));
    printk("WIFI CONNECTED\n\rWIFI RSSI: %d\n\r", status.rssi);
    return status.rssi;
}