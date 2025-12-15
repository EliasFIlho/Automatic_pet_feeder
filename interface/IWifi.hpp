#pragma once
#include <stdint.h>

class IWifi
{
public:
    virtual ~IWifi() noexcept = default;
    virtual bool wifi_init(void) = 0;
    virtual int connect_to_wifi() = 0;
    virtual int wifi_disconnect(void) = 0;
    virtual void set_credentials(const char *ssid, const char *psk) = 0;
    virtual void start_dhcp() = 0;
    virtual int32_t get_rssi() = 0;

};
