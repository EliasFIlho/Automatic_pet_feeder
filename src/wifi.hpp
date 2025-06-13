#pragma once

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>

#define PSK_BUFFER_LEN 20
#define SSID_BUFFER_LEN 20

//TODO: Add the AP mode methods for wifi credentials connection
class wifi
{
private:
    char ssid[SSID_BUFFER_LEN];
    char psk[PSK_BUFFER_LEN];


private:
    void wait_wifi_to_connect(void);
    void wifi_wait_for_ip_addr(void);


public:
    wifi();
    void wifi_init(void);
    void set_wifi_credentials(const char *ssid, const char *psk);
    int wifi_connect();
    int connect_to_wifi(char *ssid, char *psk);
    int wifi_disconnect(void);
    ~wifi();
};
