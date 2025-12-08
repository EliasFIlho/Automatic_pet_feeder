#include <zephyr/kernel.h>
#include <string.h>
#include "Application.hpp"
#include "StepperController.hpp"
#include "RTC.hpp"
#include "Storage.hpp"
#include "SchedulerRules.hpp"
#include "WifiStation.hpp"
#include "IWifi.hpp"
#include "MQTT.hpp"
#include "Led.hpp"
#include "Netmgnt.hpp"
#include "Watchdog.hpp"
#include "LvlSensor.hpp"
#include "NetEvents.hpp"
#include "JsonModule.hpp"
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(LOGS);

/**
 * @brief Define in compile time a message queue, so tasks can send publish data to MQTT
 *
 */
K_MSGQ_DEFINE(mqtt_publish_queue, sizeof(struct level_sensor), 10, 1);
K_MSGQ_DEFINE(net_evt_queue, sizeof(struct NetEventMsg), 10, 1);


/**
 * @brief Device tree devices
 *
 */
const struct device *const sensor_dev = DEVICE_DT_GET(DT_NODELABEL(hc_sr04));
const struct device *const hw_wdt_dev = DEVICE_DT_GET(DT_ALIAS(watchdog0));
struct pwm_dt_spec net_led = PWM_DT_SPEC_GET(DT_NODELABEL(fade_led));
struct gpio_dt_spec dir_stepper_dt = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
struct gpio_dt_spec steps_stepper_dt = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);
struct gpio_dt_spec enable_stepper_dt = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 2);


/**
 * @brief Objects declaration
 *
 * NOTE: I think this all should be their interface versions to make more sense
 *
 */
Watchdog guard(hw_wdt_dev);
Storage fs;
JsonModule json;
StepperController motor(&dir_stepper_dt,&steps_stepper_dt,&enable_stepper_dt);
RTC rtc;
MQTT mqtt(guard, fs, json);
Led led(&net_led);
IWifi &wifi = WifiStation::Get_Instance();
Netmgnt net(mqtt, wifi, fs, led);
Application app(rtc, motor, fs, guard);
LvlSensor sensor(sensor_dev);

// TODO: Document modules with doxygen style
// TODO: See how to use Kconfig without modify Zephyr base Kconfig

int main(void)
{
    net.Attach(&sensor);
    net.Attach(&app);
    
    LOG_INF("Start Log at main");
    if (guard.init() != 0)
    {
        LOG_ERR("Error to init watchdog\n\r");
    }
    if (fs.init_storage() != FILE_SYSTEM_ERROR::STORAGE_OK)
    {
        LOG_ERR("Error to init Fs\r\n");
        return -1;
    }
    if (net.start() == NET_ERROR::NET_OK)
    {
        LOG_INF("NET COMPONENTS STARTED");
    }
    else
    {
        LOG_ERR("NET ERROR");
    }
    app.init_application();
    sensor.init();

    while (true)
    {
        k_sleep(K_FOREVER);
    }

    return 0;
}
