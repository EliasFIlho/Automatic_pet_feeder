#pragma once

#include "StepperController.hpp"
#include "SntpClient.hpp"
#include "SchedulerRules.hpp"

class Application
{
private:
    StepperController motor;
    SntpClient sntp;
    Rules_t rules;

private:
    void get_rules(); // Acess filesystem and get the current scheduler (This function will be mocked for test since the filesystem module still not avaliable)
    void dispense_food(int amount); // Dispense a X amount of food (the amount calc will be defined after the mechanics)
    bool check_date(); // Compare current time stamp with the rules
    void app(); // Application function (This may not work bcs is class member, but i'll see how this is done)
public:
    Application();
    ~Application();
    void start_application(); // Start application thread
};
