#pragma once
#include <stdint.h>

class IMotor
{

public:
    virtual ~IMotor() noexcept = default;
    virtual void move_for(int amount) = 0;
    virtual uint32_t init() = 0;
};

