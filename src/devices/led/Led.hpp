#include "ILed.hpp"
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>



#define LED_BRIGHTNESS 16
class Led : public ILed
{
private:
    const struct device *_led;
    struct led_rgb colors;

public:
    int32_t init();
    int32_t set_output(COLOR channel, uint8_t output); // Set LED output MIN/MAX
    int32_t set_mapped_output(int32_t value,COLOR channel ,int32_t fromLow, int32_t fromHigh); // This will map the value with max and minimum led brightness and Set LED output

    Led(const struct device *const led);
    ~Led();
};
