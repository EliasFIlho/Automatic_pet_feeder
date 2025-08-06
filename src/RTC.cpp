#include "RTC.hpp"
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <time.h>
#include <arpa/inet.h>

#define SNTP_HOST "a.st1.ntp.br"
#define DATE_FORMAT "%Y-%m-%d %H:%M:%S\r\n"
#define UTC_3 10800 // 3h in seconds

RTC::RTC()
{

}

RTC::~RTC()
{
}

/**
 * @brief
 *
 * @return int
 */
int RTC::update_current_time()
{
    int ret = sntp_simple(SNTP_HOST, 4000, &s_time);

    if (ret != 0)
    {
        printk("Error to do SNTP request\r\n");
    }
    else
    {
        printk("SNTP Time: %llu\r\n", s_time.seconds);
        time_t epoch = s_time.seconds - UTC_3;
        char buff[80];
        ts = localtime(&epoch);
        ts->tm_hour = ts->tm_hour;
        strftime(buff, sizeof(buff), DATE_FORMAT, ts);
        printk("%s\r\n", buff);
    }

    return ret;
}

int RTC::get_week_day()
{
    return this->ts->tm_wday;
}

int RTC::get_day()
{
    return this->ts->tm_mday;
}
int RTC::get_month()
{
    return this->ts->tm_mon;
}
int RTC::get_year()
{
    return (this->ts->tm_year + 1900);
}

int RTC::get_hour()
{
    return this->ts->tm_hour;
}
int RTC::get_minute()
{
    return this->ts->tm_min;
}