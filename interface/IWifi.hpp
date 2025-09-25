#pragma once

class IWifi
{
public:
    virtual ~IWifi() noexcept = default;
    virtual bool wifi_init(void) = 0;
    virtual int connect_to_wifi() = 0;
    virtual int wifi_disconnect(void) = 0;
    virtual void set_wifi_ssid(char *ssid) = 0;
    virtual void set_wifi_psk(char *psk) = 0;
    virtual bool is_connected() = 0;
    virtual void init_rssi_monitor() = 0;
};
