#include "TaskRunner.hpp"


K_THREAD_STACK_DEFINE(APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE);

void TaskRunner::create_task(void (*task_func)(void *p1, void *p2, void *p3), void *param)
{
    k_thread_create(&this->task_thread, APP_STACK_AREA, CONFIG_APP_THREAD_STACK_SIZE, task_func, param, NULL, NULL, CONFIG_APP_THREAD_PRIORITY, 0, K_NO_WAIT);
}

void TaskRunner::sleep(int ms)
{
    k_msleep(ms);
}
