#include "sntp.hpp"
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <time.h>
#include <arpa/inet.h>

#define SNTP_HOST "a.st1.ntp.br"

sntp::sntp(const char *server_address)
{
    //TODO: Add server address to private server buffer
}

sntp::sntp()
{
    //TODO: Add the default BR ntp server
}

sntp::~sntp()
{
}

//TODO: Receive a char buffer pointer to copy the current date time and perhaps some struct pointer to save the date infos into a data structure
int sntp::get_current_time()
{
    int ret = sntp_simple(SNTP_HOST, 4000, &s_time);

    if (ret != 0)
    {
        printk("Error to do SNTP request\r\n");
    }
    else
    {
        printk("SNTP Time: %llu\r\n", s_time.seconds);
        time_t epoch = s_time.seconds;
        struct tm *ts;
        char buff[80];
        ts = localtime(&epoch);
        strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S\r\n", ts);
        printk("%s",buff);
        // strcpy(buffer, buff); // Copy buff into buffer
    }

    return ret;
}
