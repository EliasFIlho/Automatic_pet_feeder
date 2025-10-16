#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>  
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include "IWifi.hpp"



class WifiStation : public IWifi
{
private:
    char ssid[CONFIG_WIFI_SSID_BUF_LEN];
    char psk[CONFIG_WIFI_PSK_BUF_LEN];
    struct net_if *sta_iface;
    struct k_work_delayable rssi_monitor_work;

private:
    int wait_wifi_to_connect(void);
    int wifi_wait_for_ip_addr(void);
    static void rssi_monitor(struct k_work *work);

public:

public:
    WifiStation();
    bool wifi_init(void);
    int connect_to_wifi();
    int wifi_disconnect(void);
    void set_wifi_ssid(char *ssid);
    void set_wifi_psk(char *psk);
    bool is_connected();
    void init_rssi_monitor();
    int32_t get_rssi();


    ~WifiStation();
};
