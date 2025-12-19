#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include "IWifi.hpp"
#include "NetEvents.hpp"



class WifiStation : public IWifi
{
private:
    char ssid[CONFIG_WIFI_SSID_BUF_LEN];
    char psk[CONFIG_WIFI_PSK_BUF_LEN];
    struct net_if *sta_iface;
    struct net_mgmt_event_callback wifi_cb;
    struct net_mgmt_event_callback ipv4_cb;
    struct k_timer TIMEOUT_TMR;
    
private:
    static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);
    static void dhcp4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);
    static void timeout_tmr_handler(struct k_timer *timer_id);
    void notify_evt(Events evt);
    int wait_wifi_to_connect(void);
    int wifi_wait_for_ip_addr(void);
    
    public:
    WifiStation();
    bool wifi_init();
    int connect_to_wifi();
    int wifi_disconnect();
    void start_dhcp();
    void stop_dhcp();
    void set_credentials(const char *ssid, const char *psk);
    int32_t get_rssi();
    static WifiStation &Get_Instance();

    ~WifiStation();
};
