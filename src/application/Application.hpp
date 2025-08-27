#pragma once

#include "StepperController.hpp"
#include "RTC.hpp"
#include "SchedulerRules.hpp"
#include "WifiStation.hpp"
#include "HttpsClient.hpp"
#include "WifiSoftAp.hpp"
#include <zephyr/kernel.h>

class Application
{
private:
    struct k_thread AppTask;
    StepperController motor;
    Rules_t rules;
    WifiStation network;
    HttpsClient client;
    RTC rtc;


private:
    void get_rules(); // Acess filesystem and get the current scheduler
    void dispense_food(int amount); // Dispense a X amount of food (the amount calc will be defined after the mechanics)
    bool check_date(); // Compare current time stamp with the rules
    bool init_wifi(); // Get credentials from FS and connect to network wifi
    static void app(void *p1, void *, void *); // Application function (This may not work bcs is class member, but i'll see how this is usually done)

public:
    Application();
    ~Application();
    void start_application(); // Start application thread
};
