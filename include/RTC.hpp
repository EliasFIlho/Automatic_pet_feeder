#pragma once

#include <zephyr/net/sntp.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>

class RTC
{
private:
    struct sntp_time s_time;
    struct tm *ts;
    //const struct device * const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
public:
    RTC();
    ~RTC();
    int update_current_time();
    
    int get_week_day();
    int get_day();
    int get_month();
    int get_year();
    int get_hour();
    int get_minute();
};



