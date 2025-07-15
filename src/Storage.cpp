
#include "Storage.hpp"
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/zms.h>

#define ZMS_PARTITION storage_partition
#define ZMS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(ZMS_PARTITION)
#define ZMS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(ZMS_PARTITION)


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
        return -1;
    }
    this->fs.offset = ZMS_PARTITION_OFFSET;
    int ret = flash_get_page_info_by_offs(this->fs.flash_device, this->fs.offset, &info);
    if (ret)
    {
        printk("Unable to get page info, rc=%d\n", ret);
        return -1;
    }
    this->fs.sector_size = info.size;
    this->fs.sector_count = 3U;

    ret = zms_mount(&this->fs);
    if (ret)
    {
        printk("Storage Init failed, rc=%d\n", ret);
        return -1;
    }
    return 0;
}

int Storage::read_data(uint32_t id)
{
    char buf[BUFFER_LEN];

    int rc = zms_read(&fs, id, &buf, sizeof(buf));
    if (rc > 0)
    {
        buf[rc] = '\0';
        printk("ID: %u, IP Address: %s\r\n", id, buf);
    }
    else
    {
        printk("No data in FS\r\n");
    }
    return rc;
}
int Storage::write_data(uint32_t id, const char *data)
{

    printk("Adding IP_ADDRESS %s at id %u\n", data, id);
    int rc = zms_write(&fs, id, data, strlen(data));
    if (rc < 0)
    {
        printk("Error while writing Entry rc=%d\n", rc);
        return rc;
    }

    return rc;
}