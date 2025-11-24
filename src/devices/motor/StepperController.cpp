#include <zephyr/kernel.h>
//#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "StepperController.hpp"
#include "zephyr/logging/log.h"


LOG_MODULE_REGISTER(MOTOR_LOG);

/**
 * @brief Construct a new StepperController::StepperController object
 *
 */
StepperController::StepperController(struct gpio_dt_spec * dir, struct gpio_dt_spec * step, struct gpio_dt_spec * en) : _direction{dir}, _steps{step}, _enable{en}
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
 
    bool success = true;

    // Check if devices are ready
    if (!device_is_ready(_direction->port) || !device_is_ready(_steps->port) || !device_is_ready(_enable->port))
    {
        LOG_ERR("Device not ready!!");
        success = false;
    }

    // Check each step in sequence
    if (gpio_pin_configure_dt(this->_direction, GPIO_OUTPUT) != 0) {
        LOG_ERR("Error configuring direction pin");
        success = false;
    }
    if (gpio_pin_configure_dt(this->_steps, GPIO_OUTPUT) != 0) {
        LOG_ERR("Error configuring steps pin");
        success = false;
    }
    if (gpio_pin_configure_dt(this->_enable, GPIO_OUTPUT) != 0) {
        LOG_ERR("Error configuring enable pin");
        success = false;
    }
    if (gpio_pin_set_dt(this->_enable, 1) != 0) {
        LOG_ERR("Error setting enable pin");
        success = false;
    }

    return success ? 0 : -ENODEV;
}

/**
 * @brief Move motor to a specific position, the initial motor position will be the current shaft position at motor object constructor
 * the current position will be updated based on the steps and the direction will defined by the distance between the desired position and
 * the current position.
 *
 * @param step
 */
void StepperController::move_to(int step)
{
    int error;

    if (step < this->current_position)
    {
        gpio_pin_set_dt(_enable, 0);
        gpio_pin_set_dt(_direction, 0);
        error = this->current_position - step;
        for (int i = 0; i < error; i++)
        {
            gpio_pin_toggle_dt(_steps);
            this->current_position--;
            k_usleep(500);
        }
        gpio_pin_set_dt(_enable, 1);
    }
    else if (step > this->current_position)
    {
        gpio_pin_set_dt(_enable, 0);
        gpio_pin_set_dt(_direction, 1);
        error = step - this->current_position;
        for (int i = 0; i < error; i++)
        {
            gpio_pin_toggle_dt(_steps);
            this->current_position++;
            k_usleep(500);
        }
        gpio_pin_set_dt(_enable, 1);
    }
    else
    {
        // MISRA
    }
}

/**
 * @brief Turn motor to a fixed direction by a number of steps
 *
 * @param amout
 */
void StepperController::move_for(int amout)
{
    gpio_pin_set_dt(_direction, 0);
    gpio_pin_set_dt(_enable, 0);
    for (int i = 0; i < amout; i++)
    {
        gpio_pin_toggle_dt(_steps);
        current_position++;
        k_usleep(500);
    }
    gpio_pin_set_dt(_enable, 1);
}