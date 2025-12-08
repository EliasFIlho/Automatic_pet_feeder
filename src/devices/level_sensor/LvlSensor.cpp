#include "LvlSensor.hpp"
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

K_THREAD_STACK_DEFINE(SENSOR_STACK_AREA, CONFIG_LEVEL_SENSOR_THREAD_STACK_SIZE);
#define SENSOR_THREAD_OPTIONS (K_FP_REGS | K_ESSENTIAL)

LOG_MODULE_REGISTER(SENSOR_LOG);

LvlSensor::LvlSensor(const struct device *dev) : sensor_dev{dev}
{
}

LvlSensor::~LvlSensor()
{
}

void LvlSensor::Update(Events evt)
{
    switch (evt)
    {
    case Events::MQTT_CONNECTED:
        this->isMQTTconnectd = true;
        LOG_WRN("MQTT CONNECTED START SENDING SENSOR DATA");
        break;
    case Events::MQTT_DISCONNECTED:
        LOG_WRN("MQTT DISCONNECT STOP SENDING SENSOR DATA");
        this->isMQTTconnectd = false;
        break;
    default:
        break;
    }
}

int32_t LvlSensor::init()
{
    if (!device_is_ready(this->sensor_dev))
    {
        return -ENODEV;
    }
    else
    {
        this->sample.unit = 0;
        k_thread_create(&this->sensor_thread, SENSOR_STACK_AREA, CONFIG_LEVEL_SENSOR_THREAD_STACK_SIZE, LvlSensor::sample_sensor, this, NULL, NULL, CONFIG_LEVEL_SENSOR_THREAD_PRIORITY, SENSOR_THREAD_OPTIONS, K_NO_WAIT);
        return 0;
    }
}

void LvlSensor::get_level()
{
    struct sensor_value distance;
    int ret = sensor_sample_fetch(this->sensor_dev);

    if (ret < 0)
    {
        LOG_ERR("ERROR: Fetch failed: %d", ret);
        this->sample.value = 0;
        return;
    }
    ret = sensor_channel_get(this->sensor_dev, SENSOR_CHAN_DISTANCE, &distance);
    if (ret != 0)
    {
        LOG_ERR("ERROR: Get channel failed: %d", ret);
    }
    else
    {
        this->sample.value = distance.val1;
    }
}

int32_t LvlSensor::send_data()
{
    int ret = k_msgq_put(&mqtt_publish_queue, &this->sample, K_NO_WAIT);
    return ret;
}

void LvlSensor::sample_sensor(void *p1, void *, void *)
{
    auto *self = static_cast<LvlSensor *>(p1);
    while (true)
    {
        self->get_level();
        if (self->isMQTTconnectd)
        {
            self->send_data();
        }
        k_msleep(CONFIG_LEVEL_SENSOR_THREAD_PERIOD);
    }
}