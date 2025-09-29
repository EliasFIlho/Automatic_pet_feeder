#pragma once
#include <stdint.h>
class ILvlSensor
{

public:
    virtual ~ILvlSensor() noexcept = default;
    virtual int32_t init() = 0;
};
