#include "LvlSensor.hpp"

K_THREAD_STACK_DEFINE(SENSOR_STACK_AREA, CONFIG_LEVEL_SENSOR_THREAD_STACK_SIZE);
#define SENSOR_THREAD_OPTIONS (K_FP_REGS | K_ESSENTIAL)


//TODO: Implement sensor stuff
LvlSensor::LvlSensor()
{
}

LvlSensor::~LvlSensor()
{
}

int32_t LvlSensor::init()
{
    this->sample.unit = 0;
    k_thread_create(&this->sensor_thread, SENSOR_STACK_AREA, CONFIG_LEVEL_SENSOR_THREAD_STACK_SIZE, LvlSensor::sample_sensor, this, NULL, NULL, CONFIG_LEVEL_SENSOR_THREAD_PRIORITY, SENSOR_THREAD_OPTIONS, K_NO_WAIT);
    printk("Check init if is called\n\r");
    return 0;
}

void LvlSensor::get_level()
{
    printk("Check if is called\n\r");
    this->sample.value = 100;
}

int32_t LvlSensor::send_data(){
    int ret = k_msgq_put(&mqtt_publish_queue,&this->sample,K_NO_WAIT);
    return ret;
}

void LvlSensor::sample_sensor(void *p1, void*,void*)
{
    auto *self = static_cast<LvlSensor*>(p1);
    while(true){
        printk("Teste sensor timer sampler\n");
        self->get_level();
        self->send_data();
        k_msleep(CONFIG_LEVEL_SENSOR_THREAD_PERIOD);
    }
}