#include "ILed.hpp"
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>

class Led : public ILed
{
private:
    struct pwm_dt_spec *_led;
    uint8_t status;

public:
    int32_t init();
    int32_t set_output(uint8_t output);                                         // Set LED output MIN/MAX
    uint8_t get_output();                                                    // Return the current output led state
    int32_t set_mapped_output(int32_t value, int32_t fromLow, int32_t fromMax); // This will map the value with max and minimum led brightness and Set LED output

    Led(struct pwm_dt_spec * led);
    ~Led();
};
