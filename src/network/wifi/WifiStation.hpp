#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include "IWifi.hpp"


//TODO: Create state flow for connection process: Keep things simple ;p
enum class CON_STATE
{
    CONNECTED,
    CONNECTING,
    SCANNING,
    DISCONNECTED
};

class WifiStation : public IWifi
{
private:
    char ssid[CONFIG_WIFI_SSID_BUF_LEN];
    char psk[CONFIG_WIFI_PSK_BUF_LEN];
    CON_STATE con_state;
    struct net_if *sta_iface;
    struct k_work_delayable reconnect_k_work;
    struct net_mgmt_event_callback wifi_cb;
    struct net_mgmt_event_callback ipv4_cb;

    struct k_sem wifi_connected;
    struct k_sem ipv4_connected;

private:
    static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);
    static void dhcp4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);
    int wait_wifi_to_connect(void);
    int wifi_wait_for_ip_addr(void);
    void on_disconnect();
    static void reconnect_work(struct k_work *work);

public:
    WifiStation();
    bool wifi_init();
    int connect_to_wifi();
    int wifi_disconnect();
    void set_wifi_ssid(char *ssid);
    void set_wifi_psk(char *psk);
    bool is_connected();
    void init_rssi_monitor();
    int32_t get_rssi();
    static WifiStation &Get_Instance();

    ~WifiStation();
};
