
#include "Storage.hpp"
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/zms.h>
#include <stdlib.h>

#define ZMS_PARTITION storage_partition
#define ZMS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(ZMS_PARTITION)
#define ZMS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(ZMS_PARTITION)

Storage::Storage()
{
    //printk("Storage initialized.\r\n");
}

Storage::~Storage()
{
    //printk("Storage destroyed.\r\n");
}

FILE_SYSTEM_ERROR Storage::init_storage()
{
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
        //printk("Unable to get page info, rc=%d\n", ret);
        return FILE_SYSTEM_ERROR::STORAGE_ERROR_PAGE_INFO;
    }
    this->fs.sector_size = info.size;
    this->fs.sector_count = 3U;

    ret = zms_mount(&this->fs);
    if (ret)
    {
        //printk("Storage Init failed, rc=%d\n", ret);
        return FILE_SYSTEM_ERROR::STORAGE_ERROR_MOUNT;
    }
    return FILE_SYSTEM_ERROR::STORAGE_OK;
}

int Storage::read_data(uint32_t id, char *buf, unsigned int buf_len)
{

    int rc = zms_read(&fs, id, buf, buf_len);
    if (rc > 0)
    {
        buf[rc] = '\0';
        //printk("Data readed: %s\n\r", buf);
    }
    else
    {
        //printk("No data in FS\r\n");
    }
    return rc;
}

// TODO: Check if is valid a template for data parameter to make write method more modular
int32_t Storage::write_data(uint32_t id, const char *str)
{
    int32_t fs_free_space = this->get_free_space();
    if ((fs_free_space > 0) && (static_cast<uint32_t>(fs_free_space) < strlen(str)))
    {
        return -ENOSPC;
    }
    else if (fs_free_space < 0)
    {
        return fs_free_space;
    }
    else
    {

        int rc = zms_write(&fs, id, str, strlen(str));
        if (rc < 0)
        {
            //printk("Error while writing Entry rc=%d\n", rc);
        }
        else
        {
            //printk("Sucess to write data\r\n");
            //printk("Data writed: %s\n\r", str);
        }
        return rc;
    }
}

int32_t Storage::get_free_space()
{
    int32_t ret = zms_calc_free_space(&this->fs);
    if (ret < 0)
    {
        //printk("Error to check filesystem free space\n\r");
    }
    else
    {
        //printk("FILE SYSTEM FREE SPACE: %d\n\r", ret);
    }
    return ret;
}