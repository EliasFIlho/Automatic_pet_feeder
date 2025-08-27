#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

class StepperController
{
private:
    int current_position;
    struct gpio_dt_spec direction;
    struct gpio_dt_spec steps;
    struct gpio_dt_spec enable;

public:
    StepperController();
    ~StepperController();
    void move_to(int step);
    void move_for(int amout);
};
