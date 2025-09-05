#pragma once

#include "StepperController.hpp"
#include "RTC.hpp"
#include "SchedulerRules.hpp"
#include "WifiStation.hpp"
#include <zephyr/kernel.h>
#include "MQTT.hpp"

class Application
{
private:
    struct k_thread AppTask;
    StepperController motor;
    Rules_t rules;
    WifiStation network;
    RTC rtc;
    MQTT mqtt;


private:
    bool is_date_match(); // Check if specif date matchs with the rule selected specific date
    bool is_week_days_match(uint8_t week_day); // check if the current day matchs with the rule selected day
    bool is_time_match(); // Check if the current time matchs with the rule selected time
    int get_rules(); // Acess filesystem and get the current scheduler
    void dispense_food(); // Dispense a X amount of food (the amount calc will be defined after the mechanics)
    bool check_rules(); // Compare current time stamp with the rules
    bool init_wifi(); // Get credentials from FS and connect to network wifi
    static void app(void *p1, void *, void *); // Application function (This may not work bcs is class member, but i'll see how this is usually done)

public:
    Application();
    ~Application();
    void start_application(); // Start application thread
};
