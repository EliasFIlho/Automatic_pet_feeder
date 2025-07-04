#pragma once

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>


class WifiSoftAp
{
private:
    struct net_if *sap_iface;
    struct wifi_connect_req_params ap_config;
private:
    void populate_ap_configs();
    int enable_dhcpv4_server();
public:
    WifiSoftAp();
    int enable_soft_ap();
    ~WifiSoftAp();
};

