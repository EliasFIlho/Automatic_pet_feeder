#pragma once

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/fs/zms.h>
#include <zephyr/storage/flash_map.h>
#include "SchedulerRules.hpp"
#include "IStorage.hpp"

class Storage : public IStorage
{

private:
    struct zms_fs fs;

private:
    int32_t get_free_space();
public:
    Storage();
    ~Storage();
    FILE_SYSTEM_ERROR init_storage();
    int read_data(uint32_t id, char *buf, size_t buf_len);
    int32_t write_data(uint32_t id, const char *str);
};