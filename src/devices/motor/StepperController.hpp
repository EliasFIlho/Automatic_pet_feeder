#pragma once
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "IMotor.hpp"

class StepperController : public IMotor
{
private:
    int current_position;
    //TODO: Make these structs as pointers and get the values in main
    struct gpio_dt_spec direction;
    struct gpio_dt_spec steps;
    struct gpio_dt_spec enable;

public:
    StepperController();
    ~StepperController();
    void move_to(int step);
    void move_for(int amout);
    uint32_t init();
};
