#pragma once

#include <stdint.h>


enum{
    LOW,
    HIGH,
    FADE
};

class ILed
{
private:
public:
       virtual ~ILed() noexcept = default;
       virtual int32_t init();
       virtual int32_t set_output(uint8_t output) = 0;                                         // Set LED output MIN/MAX
       virtual uint8_t get_output() = 0;                                                    // Return the current output led state
       virtual int32_t set_mapped_output(int32_t value, int32_t fromLow, int32_t fromMax) = 0; // This will map the value with max and minimum led brightness and Set LED output
};
