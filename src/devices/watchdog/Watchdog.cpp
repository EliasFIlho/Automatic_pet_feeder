#include "Watchdog.hpp"
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(WTD_LOG);

Watchdog::Watchdog(const struct device *const hw_wdt_dev) : _hw_wdt_dev{hw_wdt_dev}
{
}

Watchdog::~Watchdog()
{
}

uint32_t Watchdog::init()
{
    int ret;
    if (!device_is_ready(this->_hw_wdt_dev))
    {
        ret = task_wdt_init(NULL);
    }
    else
    {
        ret = task_wdt_init(this->_hw_wdt_dev);
    }
    if (ret != 0)
    {
        LOG_ERR("Error to init task watchdog\n\r");
    }
    else
    {
        LOG_INF("Watchdog task inited\n\r");
    }
    return ret;
}
void Watchdog::feed(int task_wtd_id)
{
    int ret = task_wdt_feed(task_wtd_id);
    if (ret != 0)
    {
        LOG_ERR("Error to feed watchdog task in APP\n\r");
    }
    else
    {
        LOG_INF("Watchdog %d feeded", task_wtd_id);
    }
}
int Watchdog::create_and_get_wtd_timer_id(uint32_t reload_period)
{
    int task_wdt_id = task_wdt_add(reload_period, NULL, NULL);
    if (task_wdt_id != -EINVAL && task_wdt_id != ENOMEM)
    {
        LOG_INF("Task wtd created: [%d]", task_wdt_id);
    }
    else
    {
        LOG_ERR("Error to create task wtd: [%d]", task_wdt_id);
    }
    return task_wdt_id;
}