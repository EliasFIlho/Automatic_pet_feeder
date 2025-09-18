#pragma once

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dhcpv4_server.h>
#include "IWifi.hpp"

#define PSK_BUFFER_LEN 20
#define SSID_BUFFER_LEN 20

class WifiStation : public IWifi
{
private:
    char ssid[SSID_BUFFER_LEN];
    char psk[PSK_BUFFER_LEN];
    struct net_if *sta_iface;

private:
    int wait_wifi_to_connect(void);
    int wifi_wait_for_ip_addr(void);

public:
    WifiStation();
    void wifi_init(void);
    int connect_to_wifi();
    int wifi_disconnect(void);
    void set_wifi_ssid(char *ssid);
    void set_wifi_psk(char *psk);


    ~WifiStation();
};
