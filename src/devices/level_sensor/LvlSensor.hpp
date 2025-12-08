#include <zephyr/kernel.h>
#include "ILvlSensor.hpp"
#include "IListener.hpp"
#include "MQTT_utils.hpp"

extern k_msgq mqtt_publish_queue;

class LvlSensor : public IListener
{
private:
    struct k_thread sensor_thread;
    struct level_sensor sample;
    const struct device *sensor_dev;
    bool isMQTTconnectd;

private:
    void get_level();
    int32_t send_data();
    static void sample_sensor(void *p1, void *, void *);
    void Update(Events evt);

public:
    LvlSensor(const struct device * dev);
    ~LvlSensor();
    int32_t init();
};
