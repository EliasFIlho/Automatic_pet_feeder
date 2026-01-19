#include "RTC.hpp"
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(RTC_LOGS);

#define DATE_FORMAT "%Y-%m-%d %H:%M:%S\r\n"

// TODO: Move those to a Kconfig file
// TODO: Update this logic to use RTC device + SNTP to sync
// TODO: Update get methods to rtc get

#define UTC_3 10800 // 3h in seconds
#define SNTP_BASE_YEAR 1900

RTC::RTC(const struct device *const rtc) : _rtc(rtc)
{
}

RTC::~RTC()
{
}

void RTC::sync_tmr_handler(struct k_timer *timer_id)
{
    auto *self = static_cast<RTC *>(k_timer_user_data_get(timer_id));
    k_work_submit(&self->sync_work);
}

void RTC::sync_work_handler(struct k_work *work)
{
    auto *self = CONTAINER_OF(work, RTC, sync_work);
    LOG_WRN("Sync work call");
    self->sync_time();
}

int RTC::init()
{
    if (!device_is_ready(this->_rtc))
    {
        return -EIO;
    }
    k_timer_init(&this->SYNC_TMR, this->sync_tmr_handler, NULL);
    k_timer_user_data_set(&this->SYNC_TMR, this);
    k_work_init(&this->sync_work, sync_work_handler);
    this->isNetworkConnected = false;
    return 0;
}

/**
 * @brief
 *
 * @return int
 */
int RTC::sync_time()
{
    int ret;

    for (int count = 0; count < CONFIG_SNTP_TRY_COUNT; count++)
    {

        // TODO: Set timeout in a kconfig file
        ret = sntp_simple(CONFIG_SNTP_HOST, 4000, &this->s_time);

        if (ret != 0)
        {
            continue;
        }
        else
        {

            // TODO: Test this
            time_t unix_time = static_cast<time_t>(this->s_time.seconds - UTC_3);
            struct tm tm;
            gmtime_r(&unix_time, &tm);
            populate_rtc_time_spec(&tm);
            ret = rtc_set_time(this->_rtc, &this->rtc_time_spec);
            return ret;
        }
    }
    return ret;
}

void RTC::populate_rtc_time_spec(struct tm *tm)
{
    this->rtc_time_spec.tm_sec = tm->tm_sec;
    this->rtc_time_spec.tm_min = tm->tm_min;
    this->rtc_time_spec.tm_hour = tm->tm_hour;
    this->rtc_time_spec.tm_mday = tm->tm_mday;
    this->rtc_time_spec.tm_mon = tm->tm_mon;
    this->rtc_time_spec.tm_year = tm->tm_year;
    this->rtc_time_spec.tm_wday = tm->tm_wday;
    this->rtc_time_spec.tm_yday = tm->tm_yday;
    this->rtc_time_spec.tm_isdst = -1;
}

void RTC::update_time()
{
    rtc_get_time(this->_rtc, &this->rtc_time_spec);
}

/**
 * @brief Get the current week day from RTC device
 *
 * @return uint8_t [0..6] from Sunday == 0 to Saturday == 6
 */
int RTC::get_week_day()
{
    this->update_time();
    LOG_WRN("Week day %d", this->rtc_time_spec.tm_wday);
    return this->rtc_time_spec.tm_wday;
}

int RTC::get_day()
{
    this->update_time();
    LOG_WRN("Day %d", this->rtc_time_spec.tm_mday);
    return this->rtc_time_spec.tm_mday;
}
int RTC::get_month()
{
    this->update_time();
    LOG_WRN("Month %d", this->rtc_time_spec.tm_mon);
    return this->rtc_time_spec.tm_mon;
}
int RTC::get_year()
{
    this->update_time();
    LOG_WRN("Year %d", rtc_time_spec.tm_year + SNTP_BASE_YEAR);
    return (this->rtc_time_spec.tm_year + SNTP_BASE_YEAR);
}

int RTC::get_hour()
{
    this->update_time();
    LOG_WRN("Hour %d", this->rtc_time_spec.tm_hour);
    return this->rtc_time_spec.tm_hour;
}
int RTC::get_minute()
{
    this->update_time();
    LOG_WRN("Minute %d", this->rtc_time_spec.tm_min);
    return this->rtc_time_spec.tm_min;
}

void RTC::Update(Events evt)
{
    switch (evt)
    {
    case Events::IP_ACQUIRED:
        this->isNetworkConnected = true;
        k_timer_start(&this->SYNC_TMR, K_SECONDS(1), K_SECONDS(CONFIG_RTC_SYNC_TIME)); // 1 sec for first sync call, them 1h period after that;
        break;
    case Events::WIFI_DISCONNECTED:
        this->isNetworkConnected = false;
        k_timer_stop(&this->SYNC_TMR);
    default:
        break;
    }
}
