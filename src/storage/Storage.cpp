
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

enum
{
    STORAGE_ERROR_MOUNT = -3,
    STORAGE_ERROR_PAGE_INFO,
    STORAGE_ERROR_DEVICE

};

#define BUFFER_LEN 256

Storage::Storage()
{
    printk("Storage initialized.\r\n");
}

Storage::~Storage()
{
    printk("Storage destroyed.\r\n");
}

int Storage::init_storage()
{
    struct flash_pages_info info;
    this->fs.flash_device = ZMS_PARTITION_DEVICE;
    if (!device_is_ready(this->fs.flash_device))
    {
        return STORAGE_ERROR_DEVICE;
    }
    this->fs.offset = ZMS_PARTITION_OFFSET;
    int ret = flash_get_page_info_by_offs(this->fs.flash_device, this->fs.offset, &info);
    if (ret)
    {
        printk("Unable to get page info, rc=%d\n", ret);
        return STORAGE_ERROR_PAGE_INFO;
    }
    this->fs.sector_size = info.size;
    this->fs.sector_count = 3U;

    ret = zms_mount(&this->fs);
    if (ret)
    {
        printk("Storage Init failed, rc=%d\n", ret);
        return STORAGE_ERROR_MOUNT;
    }
    return 0;
}

int Storage::read_data(uint32_t id, char *buf, unsigned int buf_len)
{

    int rc = zms_read(&fs, id, buf, buf_len);
    if (rc > 0)
    {
        buf[rc] = '\0';
        //printk("Amount of readed data [%d]\r\nData: %s\n\r", rc,buf);
    }
    else
    {
        printk("No data in FS\r\n");
    }
    return rc;
}





int Storage::write_data(uint32_t id, const char *str)
{
    int rc = zms_write(&fs, id, str, strlen(str));
    if (rc < 0)
    {
        printk("Error while writing Entry rc=%d\n", rc);
    }
    else
    {
        printk("Sucess to write data\r\n");
    }
    return rc;
}

