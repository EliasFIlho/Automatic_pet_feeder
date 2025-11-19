#include <zephyr/kernel.h>
#include "ILvlSensor.hpp"
#include "MQTT_utils.hpp"
#include "INetworkEvents"

extern k_msgq mqtt_publish_queue;

class LvlSensor : public INetworkEvents
{
private:
    struct k_thread sensor_thread;
    struct level_sensor sample;
    const struct device *sensor_dev;

private:
    void get_level();
    int32_t send_data();
    static void sample_sensor(void *p1, void *, void *);
    void on_network_event(NetworkEvent evt);

public:
    LvlSensor(const struct device * dev);
    ~LvlSensor();
    int32_t init();
};
