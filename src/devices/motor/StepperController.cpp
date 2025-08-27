#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "StepperController.hpp"


/**
 * @brief Construct a new StepperController::StepperController object
 * 
 */
StepperController::StepperController()
{
    this->direction = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
    this->steps = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);
    this->enable = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 2);

    if (!device_is_ready(direction.port) || !device_is_ready(steps.port) || !device_is_ready(enable.port))
    {
        printk("Device not ready!!\r\n");
    }
    else
    {
        gpio_pin_configure_dt(&this->direction, GPIO_OUTPUT);
        gpio_pin_configure_dt(&this->steps, GPIO_OUTPUT);
        gpio_pin_configure_dt(&this->enable, GPIO_OUTPUT);
        gpio_pin_set_dt(&this->enable, 1);
    }
}

/**
 * @brief Destroy the StepperController::StepperController object
 * 
 */
StepperController::~StepperController()
{
}

/**
 * @brief 
 * 
 * @param step 
 */
void StepperController::move_to(int step)
{
    int error;

    if (step < current_position)
    {
        gpio_pin_set_dt(&enable, 0);
        gpio_pin_set_dt(&direction, 0);
        error = current_position - step;
        for (int i = 0; i < error; i++)
        {
            gpio_pin_toggle_dt(&steps);
            current_position--;
            k_usleep(500);
        }
        gpio_pin_set_dt(&enable, 1);
    }
    else if (step > current_position)
    {
        gpio_pin_set_dt(&enable, 0);
        gpio_pin_set_dt(&direction, 1);
        error = step - current_position;
        for (int i = 0; i < error; i++)
        {
            gpio_pin_toggle_dt(&steps);
            current_position++;
            k_usleep(500);
        }
        gpio_pin_set_dt(&enable, 1);
    }
    else
    {
    }
}

/**
 * @brief 
 * 
 * @param amout 
 */
void StepperController::move_for(int amout)
{
    gpio_pin_set_dt(&direction, 0);
    gpio_pin_set_dt(&enable, 0);
    for (int i = 0; i < amout; i++)
    {
        gpio_pin_toggle_dt(&steps);
        current_position++;
        k_usleep(500);
    }
    gpio_pin_set_dt(&enable, 1);
}