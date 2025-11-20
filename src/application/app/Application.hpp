#pragma once

#include "IMotor.hpp"
#include "IClock.hpp"
#include "IStorage.hpp"
#include "IWatchDog.hpp"
#include "IJson.hpp"
#include "NetworkService.hpp"
#include "SchedulerRules.hpp"
#include "ITaskRunner.hpp"
#include "INetworkEvents.hpp"
#include <array>




class Application : public INetworkEvents
{
private:
    //TODO: Need to make rules int a fixed size array and populate
    Rules_t rules;
    IClock &_clk;
    IMotor &_motor;
    IStorage &_fs;
    IWatchDog &_guard;
    IJson &_json;
    ITaskRunner &_runner;
    int task_wdt_id;
    bool isDispenserExecuted;
    bool isRulesAvaliable;


private:
    bool is_date_match();                      // Check if specif date matchs with the rule selected specific date
    bool is_week_days_match(); // check if the current day matchs with the rule selected day
    bool is_time_match();                      // Check if the current time matchs with the rule selected time
    int32_t get_rules();                           // Acess filesystem and get the current scheduler
    void dispense_food();                      // Dispense a X amount of food (the amount calc will be defined after the mechanics)
    bool check_rules();                        // Compare current time stamp with the rules
    void on_network_event(NetworkEvent evt); // Callback for network events
    void step();
    static void app(void *p1, void *, void *); // Application function

public:
    Application(IClock &clk, IMotor &motor, IStorage &fs, IWatchDog &guard, IJson &json, ITaskRunner &runner);
    ~Application();
    void init_application(); // Start application thread

};
