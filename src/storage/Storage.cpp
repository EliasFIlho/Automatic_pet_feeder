
#include "Storage.hpp"
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/zms.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <iostream>

#define ZMS_PARTITION storage_partition
#define ZMS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(ZMS_PARTITION)
#define ZMS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(ZMS_PARTITION)

LOG_MODULE_REGISTER(STORAGE_LOG);

Storage::Storage()
{
}

Storage::~Storage()
{
}

/**
 * @brief Init filesystem
 *
 * @return FILE_SYSTEM_ERROR
 */
FILE_SYSTEM_ERROR Storage::init_storage()
{
    k_mutex_init(&this->lock_mutex);
    struct flash_pages_info info;
    this->fs.flash_device = ZMS_PARTITION_DEVICE;
    if (!device_is_ready(this->fs.flash_device))
    {
        return FILE_SYSTEM_ERROR::STORAGE_ERROR_DEVICE;
    }
    this->fs.offset = ZMS_PARTITION_OFFSET;
    int ret = flash_get_page_info_by_offs(this->fs.flash_device, this->fs.offset, &info);
    if (ret)
    {
        LOG_ERR("Unable to get page info, rc=%d\n", ret);
        return FILE_SYSTEM_ERROR::STORAGE_ERROR_PAGE_INFO;
    }
    this->fs.sector_size = info.size;
    this->fs.sector_count = 3U;

    ret = zms_mount(&this->fs);
    if (ret)
    {
        LOG_ERR("Storage Init failed, rc=%d", ret);
        return FILE_SYSTEM_ERROR::STORAGE_ERROR_MOUNT;
    }
    return FILE_SYSTEM_ERROR::STORAGE_OK;
}

/**
 * @brief Generic private write function that will be used by the public calls to read from ZMS
 *
 * @param id
 * @param ptr
 * @param size
 * @return int32_t
 */
int32_t Storage::read_buffer(uint32_t id, void *ptr, size_t size)
{
    k_mutex_lock(&this->lock_mutex, K_FOREVER);
    int ret = zms_read(&this->fs, id, ptr, size);
    if (ret < 0)
    {
        LOG_ERR("ERROR TO READ DATA: %d", ret);
        k_mutex_unlock(&this->lock_mutex);
        return ret;
    }
    if (id != RULES_ID)
    {
        auto str_buffer = static_cast<char *>(ptr);
        str_buffer[ret] = '\0';
        // LOG_INF("READED DATA: %s", str_buffer);
    }
    else
    {
        LOG_INF("RULES DATA READED");
    }
    k_mutex_unlock(&this->lock_mutex);
    return ret;
}

/**
 * @brief Generic private write function that will be used by the public calls to write in ZMS
 *
 * @param id
 * @param ptr
 * @param size
 * @return int32_t
 */
int32_t Storage::write_buffer(uint32_t id, void *ptr, size_t size)
{
    k_mutex_lock(&this->lock_mutex, K_FOREVER);

    int32_t free_space = this->get_free_space();

    if (free_space < 0)
    {
        k_mutex_unlock(&this->lock_mutex);
        return free_space;
    }

    if ((uint32_t)free_space < size)
    {

        k_mutex_unlock(&this->lock_mutex);
        return -ENOSPC;
    }

    int ret = zms_write(&fs, id, ptr, size);

    if (ret != size)
    {

        LOG_ERR("Error while writing Entry ret=%d", ret);
    }
    else
    {

        LOG_INF("Success to write %zu bytes", size);
    }

    k_mutex_unlock(&this->lock_mutex);

    return ret;
}

/**
 * @brief Get ZMS free space
 *
 * @return int32_t
 */
int32_t Storage::get_free_space()
{
    int32_t ret = zms_calc_free_space(&this->fs);
    if (ret < 0)
    {
        LOG_ERR("Error to check filesystem free space");
    }
    else
    {
        LOG_INF("FILE SYSTEM FREE SPACE: %d", ret);
    }
    return ret;
} 