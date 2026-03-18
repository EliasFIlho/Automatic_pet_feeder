#pragma once

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/kvss/zms.h>
#include <zephyr/storage/flash_map.h>
#include "SchedulerRules.hpp"
#include "IStorage.hpp"

class Storage : public IStorage
{

private:
    struct zms_fs fs;
    struct k_mutex lock_mutex;

private:
    int32_t get_free_space();
    
    public:
    Storage();
    ~Storage();
    FILE_SYSTEM_ERROR init_storage();
    int32_t write_buffer(uint32_t id, void *ptr, size_t size);
    size_t read_buffer(uint32_t id, void *ptr, size_t size);
    bool is_id_empty(uint32_t id);
    int32_t delete_data(uint32_t id);
};