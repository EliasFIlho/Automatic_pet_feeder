#pragma once

#include <zephyr/net/sntp.h>

#define SERVER_MAX_LEN 20

class sntp
{
private:
    struct sntp_time s_time;
    char server[SERVER_MAX_LEN];
public:
    sntp();
    sntp(const char *server);
    ~sntp();
    int get_current_time();

};



