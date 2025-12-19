#pragma once

#include <stdint.h>

enum
{
    LOW,
    HIGH,
    FADE
};

enum class COLOR
{
    RED,
    GREEN,
    BLUE,
    YELLOW
};

class ILed
{
private:
public:
    virtual ~ILed() noexcept = default;
    virtual int32_t init() = 0;
    virtual int32_t set_output(COLOR channel, uint8_t output) = 0;
    virtual int32_t set_mapped_output(int32_t value,COLOR channel ,int32_t fromLow, int32_t fromHigh) = 0;
};
