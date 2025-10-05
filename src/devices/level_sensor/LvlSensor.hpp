#include <zephyr/kernel.h>
#include "ILvlSensor.hpp"
#include "MQTT_utils.hpp"

extern k_msgq mqtt_publish_queue;

class LvlSensor
{
private:
    struct k_thread sensor_thread;
    struct level_sensor sample;
private:
    void get_level();
    int32_t send_data();
    static void sample_sensor(void *p1, void *, void *);


public:
    LvlSensor();
    ~LvlSensor();
    int32_t init();
};
