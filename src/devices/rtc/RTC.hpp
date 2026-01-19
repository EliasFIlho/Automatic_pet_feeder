#pragma once

#include <zephyr/net/sntp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include "IRTC.hpp"
#include "IListener.hpp"

class RTC : public IRTC, public IListener
{
private:
    struct sntp_time s_time;
    struct timespec time_spec;
    struct timeval tv;
    struct tm tm;
    struct rtc_time rtc_time_spec;
    const struct device *_rtc;
    bool isNetworkConnected;
    struct k_timer SYNC_TMR;
    struct k_work sync_work;

private:
    void update_time();
    void populate_rtc_time_spec(struct tm *tm);
    static void sync_tmr_handler(struct k_timer *timer_id);
    static void sync_work_handler(struct k_work *work);
    int sync_time();

public:
    RTC(const struct device *const rtc);
    ~RTC();
    int get_week_day(); // Function to parse and return the week day
    int get_day();      // Function to parse and return the month day
    int get_month();    // Function to parse and return the month
    int get_year();     // Function to parse and return the year
    int get_hour();     // Function to parse and return the hour
    int get_minute();   // Function to parse and return the minutes
    void Update(Events evt);
    int init();
};
