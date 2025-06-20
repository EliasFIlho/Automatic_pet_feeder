#pragma once

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>

#define PSK_BUFFER_LEN 20
#define SSID_BUFFER_LEN 20

//TODO: Add the AP mode methods for wifi credentials connection
class Wifi_Station
{
private:
    char ssid[SSID_BUFFER_LEN];
    char psk[PSK_BUFFER_LEN];
    struct net_if *sta_iface;
    


private:
    int wait_wifi_to_connect(void);
    int wifi_wait_for_ip_addr(void);


public:
    Wifi_Station();
    void wifi_init(void);
    void set_wifi_credentials(const char *ssid, const char *psk);
    int wifi_connect();
    int connect_to_wifi(char *ssid, char *psk);
    int wifi_disconnect(void);
    void get_connection_status(void);


    ~Wifi_Station();
};
