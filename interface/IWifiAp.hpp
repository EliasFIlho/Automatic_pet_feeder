#pragma once
#include <stdint.h>

class IWifiAp
{
public:
    virtual ~IWifiAp() noexcept = default;
    virtual void ap_init(void) = 0;
    virtual void ap_start(void) = 0;
    virtual void ap_stop(void) = 0;
    virtual bool is_ap_enabled(void) = 0;
};
