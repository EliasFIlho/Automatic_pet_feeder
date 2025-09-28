#include "Led.hpp"

Led::Led()
{
}

Led::~Led()
{
}

int32_t Led::init()
{
    this->led = PWM_DT_SPEC_GET(DT_NODELABEL(fade_led));
    if (!device_is_ready(this->led.dev))
    {
        printk("Error: PWM device %s is not ready\n", this->led.dev->name);
        return -EIO;
    }else{
        return 0;
    }
}
int32_t Led::set_output(uint8_t output)
{
    if (output == LOW || output == HIGH)
    {

        this->status = output;
        return pwm_set_pulse_dt(&this->led, output * 1000);
    }
    else
    {
        return -EINVAL;
    }
}
uint8_t Led::get_output()
{
    return this->status;
}
int32_t Led::set_mapped_output(int32_t value, int32_t fromLow = -90, int32_t fromMax = -30)
{
    this->status = FADE;
    if (value < 0)
    {
        value *= -1;
    }
    const int32_t range = (fromMax - fromLow);
    const uint64_t out = (((value*100)/range)*10000)/100;
    return pwm_set_pulse_dt(&this->led, out);
}