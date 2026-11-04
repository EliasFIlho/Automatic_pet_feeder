#pragma once
#include "types.hpp"
class IBluetoothLE
{
public:
    virtual ~IBluetoothLE() noexcept = default;
    virtual int init(provisioning_callback callback_func) = 0;
    virtual int adv_start() = 0;
    virtual int adv_stop() = 0;
    virtual int stop() = 0;
    virtual void notify_connection_state(uint8_t status) = 0;
};
