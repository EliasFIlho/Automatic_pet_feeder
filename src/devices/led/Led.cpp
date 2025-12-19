#include "Led.hpp"
#include "zephyr/logging/log.h"
#include <zephyr/drivers/led_strip.h>

LOG_MODULE_REGISTER(LED_LOG);
#define RGB(_r, _g, _b) {.r = (_r), .g = (_g), .b = (_b)}

// TODO: Improve LED module to work with led_strip module/driver

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

int32_t Led::set_output(COLOR channel, uint8_t output)
{
    switch (channel)
    {
    case COLOR::RED:
        this->colors = RGB(output, 0, 0);

        break;
    case COLOR::GREEN:
        this->colors = RGB(0, output, 0);

        break;
    case COLOR::BLUE:
        this->colors = RGB(0, 0, output);

        break;
    case COLOR::YELLOW:
        this->colors = RGB(output, output, 0);
        break;

    default:
        this->colors = RGB(255, 255, 255);
        break;
    }
    int ret = led_strip_update_rgb(this->_led, &this->colors, 1);
    if (ret == 0)
    {
        LOG_WRN("LED STRIP UPDATED");
    }
    else
    {
        LOG_ERR("LED STRIP ERROR TO UPDATE");
    }
    return ret;
}

//
int32_t Led::set_mapped_output(int32_t value, COLOR channel, int32_t fromLow = -100, int32_t fromHigh = -30)
{
    if (value > fromHigh)
    {
        value = fromHigh;
    }
    else if (value < fromLow)
    {
        value = fromLow;
    }

    int32_t out = ((value - fromLow) * 255) / (fromHigh - fromLow);
    LOG_WRN("Led Output after map: %d",out);
    this->set_output(channel, out);
    return 0;
}