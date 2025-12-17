#include "Led.hpp"
#include "zephyr/logging/log.h"
#include <zephyr/drivers/led_strip.h>

LOG_MODULE_REGISTER(LED_LOG);
#define RGB(_r, _g, _b) {.r = (_r), .g = (_g), .b = (_b)}


//TODO: Improve LED module to work with led_strip module/driver

Led::Led(const struct device *const led) : _led(led)
{
}

Led::~Led()
{
}

int32_t Led::init()
{
    if (!device_is_ready(this->_led))
    {
        LOG_ERR("Error: PWM device %s is not ready\n", this->_led->name);
        return -EIO;
    }
    else
    {
        return 0;
    }
}

//TODO: Add parameter to receive channel and value (color, brightness)
int32_t Led::set_output(uint8_t output)
{
    switch (output)
    {
    case RED:
        this->colors = RGB(255, 0, 0);

        break;
    case GREEN:
        this->colors = RGB(0, 255, 0);

        break;
    case BLUE:
        this->colors = RGB(0, 0, 255);

        break;
    case YELLOW:
        this->colors = RGB(255, 255, 0);
        break;

    default:
        this->colors = RGB(255, 255, 255);
        break;
    }

    if (led_strip_update_rgb(this->_led, &this->colors, 1) == 0)
    {
        LOG_WRN("LED STRIP UPDATED");
    }
    else
    {
        LOG_ERR("LED STRIP ERROR TO UPDATE");
    }
    return 0;
}

//
int32_t Led::set_mapped_output(int32_t value, int32_t fromLow = -90, int32_t fromMax = -30)
{
    if (value < 0)
    {
        value *= -1;
    }
    const int32_t range = (fromMax - fromLow);
    const uint64_t out = (((value * 100) / range) * 10000) / 100;
    this->set_output(GREEN);
    return 0;
}