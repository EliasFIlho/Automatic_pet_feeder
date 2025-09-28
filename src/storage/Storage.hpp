#pragma once

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/fs/zms.h>
#include <zephyr/storage/flash_map.h>
#include "SchedulerRules.hpp"
#include "IStorage.hpp"


enum 
{
    STORAGE_ERROR_MOUNT = -3,
    STORAGE_ERROR_PAGE_INFO,
    STORAGE_ERROR_DEVICE

};

class Storage : public IStorage
{

private:
    struct zms_fs fs;
    static Storage instance;

private:
public:
    //static Storage &getInstance();
    Storage();
    ~Storage();
    int init_storage();
    int read_data(uint32_t id, char *buf, size_t buf_len);
    int write_data(uint32_t id, const char *str);
};