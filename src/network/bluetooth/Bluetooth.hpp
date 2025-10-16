#pragma once
#include <zephyr/kernel.h>

/**
 * @brief Bluetooth class
 * 
 * At the date of this comment (10/14/25) the device will only use bluetooth to config wifi credentials, beside this, all communication will be
 * handled by wifi + mqtt.
 * 
 */

class Bluetooth
{
private:

public:
    // Config settings that bluetooth needs to run
    int32_t init_bluetooth();
    // Enable bluetooth callbacks and advertise device over radio module
    void enable_bluetooth();
    // Stop advertise and turn off radio for power saving (Although there are no planning for battery yet)
    void disable_bluetooth();
    Bluetooth();
    ~Bluetooth();
};
