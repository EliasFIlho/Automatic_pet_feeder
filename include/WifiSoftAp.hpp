#pragma once



class WifiSoftAp
{
private:
    void enable_dhcpv4_server();
public:
    WifiSoftAp();
    int enable_soft_ap();
    ~WifiSoftAp();
};

