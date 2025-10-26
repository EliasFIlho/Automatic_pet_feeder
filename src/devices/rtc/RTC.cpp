#include "RTC.hpp"
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <arpa/inet.h>
#include <time.h>
#include <zephyr/posix/time.h>
#include <sys/time.h>

#define DATE_FORMAT "%Y-%m-%d %H:%M:%S\r\n"

//TODO: Move those to a Kconfig file
#define SNTP_HOST "a.st1.ntp.br"
#define UTC_3 10800 // 3h in seconds

#define SNTP_TRY_COUNT 5
#define SNTP_BASE_YEAR 1900


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
            //TODO: Set timeout in a kconfig file
            ret = sntp_simple(SNTP_HOST, 4000, &this->s_time);

            if (ret != 0)
            {
                count++;
                //printk("Error to do SNTP request retry count [%d]\r\n", count);
            }
            else
            {
                this->time_spec.tv_sec = (this->s_time.seconds - UTC_3);
                this->time_spec.tv_nsec = (this->s_time.seconds * 1000000000ULL) >> 32; //SET NANO SECS
                clock_settime(CLOCK_REALTIME, &this->time_spec);
                break;
            }
        }
    }

    return ret;
}

int RTC::get_epoch()
{
    return this->s_time.seconds;
}

void RTC::update_time()
{
    int ret = gettimeofday(&this->tv, NULL);
    if (ret < 0)
    {
        //printf("Error in gettimeofday(): %d\n", errno);
    }
    time_t now = time(NULL);
    gmtime_r(&now, &this->tm);
}

/**
 * @brief Get the current week day from RTC device
 *
 * @return uint8_t [0..6] from Sunday == 0 to Saturday == 6
 */
int RTC::get_week_day()
{
    this->update_time();
    return this->tm.tm_wday;
}

int RTC::get_day()
{
    this->update_time();
    return this->tm.tm_mday;
}
int RTC::get_month()
{
    this->update_time();
    return this->tm.tm_mon;
}
int RTC::get_year()
{
    this->update_time();
    return (this->tm.tm_year + SNTP_BASE_YEAR);
}

int RTC::get_hour()
{
    this->update_time();
    return this->tm.tm_hour;
}
int RTC::get_minute()
{
    this->update_time();
    return this->tm.tm_min;
}
