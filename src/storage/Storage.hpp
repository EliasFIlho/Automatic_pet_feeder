#pragma once

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/fs/zms.h>
#include <zephyr/storage/flash_map.h>
#include "SchedulerRules.hpp"

#define SSID_ID 0
#define PASSWORD_ID 1
#define RULES_ID 2

class Storage
{

private:
    struct zms_fs fs;
    static Storage instance;

private:
    Storage();
    ~Storage();

public:
    static Storage &getInstance();
    int init_storage();
    int read_data(uint32_t id, char *buf, size_t buf_len);
    int read_data(uint32_t id, Rules_t &buf);
    int write_data(uint32_t id, const char *str);
    int write_data(uint32_t id, const Rules_t &rules);
};