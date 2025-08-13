#include "RTC.hpp"
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <arpa/inet.h>
#include <time.h>
#include <zephyr/posix/time.h>

#define SNTP_HOST "a.st1.ntp.br"
#define DATE_FORMAT "%Y-%m-%d %H:%M:%S\r\n"
#define UTC_3 10800 // 3h in seconds

#define SNTP_TRY_COUNT 5

RTC::RTC()
{
}

RTC::~RTC()
{
}

static inline void rtc_from_tm(struct rtc_time *rtc_st, const struct tm *tm_st)
{
    rtc_st->tm_sec = tm_st->tm_sec;
    rtc_st->tm_min = tm_st->tm_min;
    rtc_st->tm_hour = tm_st->tm_hour;
    rtc_st->tm_mday = tm_st->tm_mday;
    rtc_st->tm_mon = tm_st->tm_mon;
    rtc_st->tm_year = tm_st->tm_year;
    rtc_st->tm_wday = tm_st->tm_wday;
    rtc_st->tm_yday = tm_st->tm_yday;
    rtc_st->tm_isdst = tm_st->tm_isdst;
}

/**
 * @brief
 *
 * @return int
 */
int RTC::sync_time()
{
    int ret;
    int count = 0;
    while (count < SNTP_TRY_COUNT)
    {
        if (count >= SNTP_TRY_COUNT)
        {
            return -1;
        }
        else
        {
            ret = sntp_simple(SNTP_HOST, 4000, &this->s_time);

            if (ret != 0)
            {
                count++;
                printk("Error to do SNTP request retry count [%d]\r\n",count);
            }
            else
            {
                break;
            }
        }
    }

    struct rtc_time tm;
    struct tm *ts;
    time_t epoch = s_time.seconds - UTC_3;
    ts = localtime(&epoch);
    // printk("Tm struct date\r\nW_D[%d] - Y[%d] - M_D[%d]\n\r", ts->tm_wday, ts->tm_year, ts->tm_mday);
    rtc_from_tm(&tm, ts);
    // printk("rtc_time struct date\r\nW_D[%d] - Y[%d] - M_D[%d]\n\r", tm.tm_wday,tm.tm_year,tm.tm_mday);
    rtc_set_time(this->rtc, &tm);

    return ret;
}

int RTC::get_week_day()
{
    struct rtc_time tm;
    int ret = rtc_get_time(this->rtc, &tm);
    if (ret < 0)
    {
        printk("Error to get time from RTC\r\n");
        return ret;
    }
    else
    {
        printk("rtc_time struct date\r\nW_D[%d] - Y[%d] - M_D[%d]\n\r", tm.tm_wday, tm.tm_year, tm.tm_mday);
        return tm.tm_wday;
    }
}

int RTC::get_day()
{
}
int RTC::get_month()
{
}
int RTC::get_year()
{
}

int RTC::get_hour()
{
}
int RTC::get_minute()
{
}