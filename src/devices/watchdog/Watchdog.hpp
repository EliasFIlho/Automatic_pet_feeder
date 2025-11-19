#pragma once
#include "IWatchDog.hpp"
#include <zephyr/kernel.h>
class Watchdog : public IWatchDog
{
private:
    const struct device *_hw_wdt_dev;
public:
    Watchdog(const struct device *const hw_wdt_dev);
    uint32_t init();
    void feed(int task_wtd_id);
    int create_and_get_wtd_timer_id(uint32_t reload_period);
    ~Watchdog();
};