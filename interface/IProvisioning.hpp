#pragma once
#include "types.hpp"

class IProvisioning
{

public:
    virtual ~IProvisioning() noexcept = default;
    virtual int start_provisioning() = 0;
    virtual int stop_provisioning() = 0;
    virtual bool isProvisioningEnabled_() = 0;
    virtual void notify_connection_status(uint8_t status) = 0;
    virtual void write_credentials() = 0;
};
