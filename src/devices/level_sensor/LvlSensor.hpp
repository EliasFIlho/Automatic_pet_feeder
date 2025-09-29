#include <zephyr/kernel.h>
#include "ILvlSensor.hpp"

class LvlSensor
{
private:
    struct k_thread sensor_thread;

    uint8_t get_level();
    static void sample_sensor(void *p1, void*,void*);

public:
    LvlSensor();
    ~LvlSensor();
    int32_t init();
};
