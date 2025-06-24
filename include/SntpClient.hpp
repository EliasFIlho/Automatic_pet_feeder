#pragma once

#include <zephyr/net/sntp.h>


#define SERVER_MAX_LEN 20

class SntpClient
{
private:
    struct sntp_time s_time;
    struct tm *ts;
    char server[SERVER_MAX_LEN];
public:
    SntpClient();
    SntpClient(const char *server);
    ~SntpClient();
    int update_current_time();
    
    int get_week_day();
    int get_day();
    int get_month();
    int get_year();
    int get_hour();
    int get_minute();
};



