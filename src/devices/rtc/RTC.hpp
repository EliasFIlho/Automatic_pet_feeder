#pragma once

#include <zephyr/net/sntp.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include "IRTC.hpp"

class RTC : public IRTC
{
private:
    struct sntp_time s_time;
    struct timespec time_spec;
    struct timeval tv;
    struct tm tm;
    
private:
    void update_time();
public:
    RTC();
    ~RTC();
    int sync_time(); // Function to sync wall-clock time
    int get_week_day(); // Function to parse and return the week day
    int get_epoch();
    int get_day(); // Function to parse and return the month day
    int get_month(); //Function to parse and return the month
    int get_year(); //Function to parse and return the year
    int get_hour(); //Function to parse and return the hour
    int get_minute();//Function to parse and return the minutes

};



