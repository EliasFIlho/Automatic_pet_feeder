#pragma once
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "IMotor.hpp"

class StepperController : public IMotor
{
private:
    int current_position;
    struct gpio_dt_spec *_direction;
    struct gpio_dt_spec *_steps;
    struct gpio_dt_spec *_enable;

public:
    StepperController(struct gpio_dt_spec * dir, struct gpio_dt_spec * step, struct gpio_dt_spec * en);
    ~StepperController();
    void move_to(int step);
    void move_for(int amout);
    uint32_t init();
};
