/*
 *
 * Init Wifi and send a HTTP GET request to google
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "Application.hpp"
#include "TaskRunner.hpp"
#include "StepperController.hpp"
#include "RTC.hpp"
#include "Storage.hpp"
#include "SchedulerRules.hpp"
#include "WifiStation.hpp"
#include "MQTT.hpp"
#include "Led.hpp"
#include "NetworkService.hpp"
#include "Watchdog.hpp"
#include "LvlSensor.hpp"
#include "JsonModule.hpp"
#include <zephyr/task_wdt/task_wdt.h>


/**
 * @brief Define in compile time a message queue, so tasks can send publish data to MQTT perform
 *
 */
K_MSGQ_DEFINE(mqtt_publish_queue, sizeof(struct level_sensor), 10, 1);


Watchdog guard;
Storage fs;
JsonModule json;
StepperController motor;
RTC rtc;
WifiStation wifi;
MQTT mqtt(guard, fs, json);
Led led;
NetworkService net(mqtt, wifi, fs, led);
TaskRunner task_runner;
Application app(rtc, motor, fs, guard, json, task_runner);
LvlSensor sensor;

//TODO: Convert periods and configs constants to Kconfig flags

int main(void)
{

    if(guard.init() != 0){
        printk("Error to init watchdog\n\r");
    }

    int ret = fs.init_storage();
    if (ret != 0)
    {
        printk("Error to init Fs\r\n");
        return -1;
    }
    if (net.start())
    {
        app.init_application();
        sensor.init();
    }else{
        printk("NET ERROR\n\r");
    }
    
    while (true)
    {
        k_sleep(K_FOREVER);
    }

    return 0;
}
