#pragma once

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/fs/zms.h>
#include <zephyr/storage/flash_map.h>


#define SSID_ID 0
#define PASSWORD_ID 1
#define RULES_ID 2

class Storage {

private:
    struct zms_fs fs;


public:
    int init_storage();
    int read_data(uint32_t id);
    int write_data(uint32_t id, const char *data);
    Storage();
    ~Storage();
};