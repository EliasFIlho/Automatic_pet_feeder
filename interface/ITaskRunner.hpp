#pragma once

class ITaskRunner
{
public:
    virtual void create_task(void (*task_func)(void *p1, void *p2, void *p3), void *param) = 0;
    virtual void sleep(int ms) = 0;
};
