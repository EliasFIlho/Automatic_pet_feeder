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
}

/**
 * @brief Destroy the StepperController::StepperController object
 *
 */
StepperController::~StepperController()
{
}

uint32_t StepperController::init()
{
    this->direction = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
    this->steps = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);
    this->enable = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 2);

    bool success = true;

    // Check if devices are ready
    if (!device_is_ready(direction.port) || !device_is_ready(steps.port) || !device_is_ready(enable.port))
    {
        printk("Device not ready!!\r\n");
        success = false;
    }

    // Check each step in sequence
    if (gpio_pin_configure_dt(&this->direction, GPIO_OUTPUT) != 0) {
        printk("Error configuring direction pin\r\n");
        success = false;
    }
    if (gpio_pin_configure_dt(&this->steps, GPIO_OUTPUT) != 0) {
        printk("Error configuring steps pin\r\n");
        success = false;
    }
    if (gpio_pin_configure_dt(&this->enable, GPIO_OUTPUT) != 0) {
        printk("Error configuring enable pin\r\n");
        success = false;
    }
    if (gpio_pin_set_dt(&this->enable, 1) != 0) {
        printk("Error setting enable pin\r\n");
        success = false;
    }

    return success ? 0 : -ENODEV;
}

/**
 * @brief
 *
 * @param step
 */
void StepperController::move_to(int step)
{
    int error;

    if (step < this->current_position)
    {
        gpio_pin_set_dt(&enable, 0);
        gpio_pin_set_dt(&direction, 0);
        error = this->current_position - step;
        for (int i = 0; i < error; i++)
        {
            gpio_pin_toggle_dt(&steps);
            this->current_position--;
            k_usleep(500);
        }
        gpio_pin_set_dt(&enable, 1);
    }
    else if (step > this->current_position)
    {
        gpio_pin_set_dt(&enable, 0);
        gpio_pin_set_dt(&direction, 1);
        error = step - this->current_position;
        for (int i = 0; i < error; i++)
        {
            gpio_pin_toggle_dt(&steps);
            this->current_position++;
            k_usleep(500);
        }
        gpio_pin_set_dt(&enable, 1);
    }
    else
    {
        // MISRA
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