#include "Watchdog.hpp"
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/kernel.h>

Watchdog::Watchdog()
{
}

Watchdog::~Watchdog()
{
}

void Watchdog::init()
{
    int ret;
    ret = task_wdt_init(NULL);
    if (ret != 0)
    {
        printk("Error to init task watchdog\n\r");
    }
    else
    {
        printk("Watchdog task inited\n\r");
    }
}
void Watchdog::feed(int task_wtd_id)
{
    int ret = task_wdt_feed(task_wtd_id);
    if (ret != 0)
    {
        printk("Error to feed watchdog task in APP\n\r");
    }
}
int Watchdog::create_and_get_wtd_timer_id(uint32_t reload_period)
{
    int task_wdt_id = task_wdt_add(reload_period, NULL, NULL);
    if (task_wdt_id != -EINVAL && task_wdt_id != ENOMEM)
    {
        printk("Task wtd created: [%d]\n\r", task_wdt_id);
    }
    else
    {
        printk("Error to create task wtd: [%d]\n\r", task_wdt_id);
    }
    return task_wdt_id;
}