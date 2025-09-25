#pragma once
#include <stdint.h>
class IWatchDog
{
private:
    /* data */
public:
    virtual ~IWatchDog() noexcept = default;
    virtual uint32_t init() = 0;
    virtual void feed(int task_wtd_id) = 0;
    virtual int create_and_get_wtd_timer_id(uint32_t reload_period) = 0;
};

