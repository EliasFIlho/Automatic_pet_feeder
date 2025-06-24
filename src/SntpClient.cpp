#include "SntpClient.hpp"
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <time.h>
#include <arpa/inet.h>

#define SNTP_HOST "a.st1.ntp.br"
#define DATE_FORMAT "%Y-%m-%d %H:%M:%S\r\n"
#define UTC_3 10800 // 3h in seconds

SntpClient::SntpClient(const char *server_address)
{
    // TODO: Add server address to private server buffer
}

SntpClient::SntpClient()
{
    // TODO: Add the default BR ntp server
}

SntpClient::~SntpClient()
{
}

/**
 * @brief
 *
 * @return int
 */
int SntpClient::update_current_time()
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

int SntpClient::get_week_day()
{
    return this->ts->tm_wday;
}

int SntpClient::get_day()
{
    return this->ts->tm_mday;
}
int SntpClient::get_month()
{
    return this->ts->tm_mon;
}
int SntpClient::get_year()
{
    return (this->ts->tm_year + 1900);
}

int SntpClient::get_hour()
{
    return this->ts->tm_hour;
}
int SntpClient::get_minute()
{
    return this->ts->tm_min;
}