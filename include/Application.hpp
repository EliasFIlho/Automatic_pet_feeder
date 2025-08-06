#pragma once

#include "StepperController.hpp"
#include "RTC.hpp"
#include "SchedulerRules.hpp"

class Application
{
private:
    StepperController motor;
    RTC rtc;
    Rules_t rules;

private:
    void get_rules(); // Acess filesystem and get the current scheduler
    void dispense_food(int amount); // Dispense a X amount of food (the amount calc will be defined after the mechanics)
    bool check_date(); // Compare current time stamp with the rules
    void app(); // Application function (This may not work bcs is class member, but i'll see how this is usually done)
public:
    Application();
    ~Application();
    void start_application(); // Start application thread
};
