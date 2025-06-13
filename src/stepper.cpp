#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "stepper.hpp"

// static const struct gpio_dt_spec direction = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
// static const struct gpio_dt_spec steps = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

//TODO: Raise some exceptions if node fails to init
stepper::stepper()
{
    direction = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
    steps = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);

    if (!device_is_ready(direction.port) || !device_is_ready(steps.port))
    {
        printk("Device not ready!!\r\n");
    }
    else
    {
        gpio_pin_configure_dt(&direction, GPIO_OUTPUT);
        gpio_pin_configure_dt(&steps, GPIO_OUTPUT);
    }
}

stepper::~stepper()
{
}

void stepper::move_to(int step)
{
    int error;

    if (step < current_position)
    {
        gpio_pin_set_dt(&direction, 0);
        error = current_position - step;
        for (int i = 0; i < error; i++)
        {
            gpio_pin_toggle_dt(&steps);
            current_position--;
            k_usleep(500);
        }
    }
    else if (step > current_position)
    {
        gpio_pin_set_dt(&direction, 1);
        error = step - current_position;
        for (int i = 0; i < error; i++)
        {
            gpio_pin_toggle_dt(&steps);
            current_position++;
            k_usleep(500);
        }
    }
    else
    {
    }
}

// Always clock wise
void stepper::move_for(int amout)
{
    gpio_pin_set_dt(&direction, 0);
    for (int i = 0; i < amout; i++)
    {
        gpio_pin_toggle_dt(&steps);
        current_position++;
        k_usleep(500);
    }
}