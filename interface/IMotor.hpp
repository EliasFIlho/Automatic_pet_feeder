#pragma once
#include <stdint.h>

class IMotor
{

public:
    virtual ~IMotor() noexcept = default;
    virtual void move_to(int step) = 0;
    virtual void move_for(int amout) = 0;
};

