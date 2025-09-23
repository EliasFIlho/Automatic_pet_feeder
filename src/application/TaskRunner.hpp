#pragma once

#include <zephyr/kernel.h>
#include "ITaskRunner.hpp"

class TaskRunner : public ITaskRunner
{
public:
    void create_task(void (*task_func)(void *p1, void *p2, void *p3), void *param);
    void sleep(int ms);
    void feed_watchdog(int task_wdt_id);

private:
    struct k_thread task_thread;
};