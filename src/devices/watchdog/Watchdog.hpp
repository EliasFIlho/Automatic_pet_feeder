#pragma once
#include "IWatchDog.hpp"
class Watchdog : public IWatchDog
{
private:
public:
    Watchdog();
    uint32_t init();
    void feed(int task_wtd_id);
    int create_and_get_wtd_timer_id(uint32_t reload_period);
    ~Watchdog();
};