#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

class stepper
{
private:
    int current_position;
    struct gpio_dt_spec direction;
    struct gpio_dt_spec steps;

public:
    stepper();
    ~stepper();
    void move_to(int step);
    void move_for(int amout);
};
