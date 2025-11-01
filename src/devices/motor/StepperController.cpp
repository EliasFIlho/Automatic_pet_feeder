#include <zephyr/kernel.h>
//#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "StepperController.hpp"
#include "zephyr/logging/log.h"


LOG_MODULE_REGISTER(MOTOR_LOG);


//TODO: Considere to make motor spin as k_work to decrease spin speed and do not affect other threads


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
    //TODO: Move these macro calls to main and pointer assign to constructor
    this->direction = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
    this->steps = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);
    this->enable = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 2);

    bool success = true;

    // Check if devices are ready
    if (!device_is_ready(direction.port) || !device_is_ready(steps.port) || !device_is_ready(enable.port))
    {
        LOG_ERR("Device not ready!!");
        success = false;
    }

    // Check each step in sequence
    if (gpio_pin_configure_dt(&this->direction, GPIO_OUTPUT) != 0) {
        LOG_ERR("Error configuring direction pin");
        success = false;
    }
    if (gpio_pin_configure_dt(&this->steps, GPIO_OUTPUT) != 0) {
        LOG_ERR("Error configuring steps pin");
        success = false;
    }
    if (gpio_pin_configure_dt(&this->enable, GPIO_OUTPUT) != 0) {
        LOG_ERR("Error configuring enable pin");
        success = false;
    }
    if (gpio_pin_set_dt(&this->enable, 1) != 0) {
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
 * @brief Turn motor to a fixed direction by a number of steps
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