#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include "NetEvents.hpp"
#include "Enums.hpp"
#include "IWifiAp.hpp"

class WifiAp : public IWifiAp
{
private:
    struct net_if *ap_iface;
    struct wifi_connect_req_params ap_config;
    struct net_mgmt_event_callback ap_cb;
    struct in_addr addr;
    struct in_addr netmaskAddr;
    bool isSoftAPEnabled; // Will I need this flag? Probably not, but I’ll keep it anyway.

private:
    void notify_evt(Events evt);
    static void wifi_ap_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);
    void config_dhcp_server();

public:
    void ap_init(void);
    void ap_start(void);
    void ap_stop(void);
    bool is_ap_enabled(void);

    WifiAp();
    ~WifiAp();
};
