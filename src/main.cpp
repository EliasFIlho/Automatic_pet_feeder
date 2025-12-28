#include <zephyr/kernel.h>
#include <string.h>
#include "Application.hpp"
#include "StepperController.hpp"
#include "RTC.hpp"
#include "Storage.hpp"
#include "SchedulerRules.hpp"
#include "WifiStation.hpp"
#include "WifiAp.hpp"
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
K_MSGQ_DEFINE(net_evt_queue, sizeof(struct EventMsg), 10, 1);

/**
 * @brief Device tree devices
 *
 */
const struct device *const sensor_dev = DEVICE_DT_GET(DT_NODELABEL(hc_sr04));
const struct device *const hw_wdt_dev = DEVICE_DT_GET(DT_ALIAS(watchdog0));
const struct device *const net_led = DEVICE_DT_GET(DT_ALIAS(led_strip));

struct gpio_dt_spec dir_stepper_dt = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 0);
struct gpio_dt_spec steps_stepper_dt = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 1);
struct gpio_dt_spec enable_stepper_dt = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(stepper), gpios, 2);

/**
 * @brief Objects declaration
 *
 */
Watchdog guard(hw_wdt_dev);
Storage fs;
JsonModule json;
StepperController motor(&dir_stepper_dt, &steps_stepper_dt, &enable_stepper_dt);
RTC rtc;
MQTT mqtt(guard, fs, json);
Led led(net_led);
WifiStation wifi(fs);
WifiAp soft_ap;
Netmgnt net(mqtt, wifi, led,soft_ap);
Application app(rtc, motor, fs, guard);
LvlSensor sensor(sensor_dev);

// TODO: Document modules with doxygen style
// TODO: See how to use Kconfig without modify Zephyr base Kconfig

int main(void)
{

    __ASSERT(guard.init() == 0, "Error to init watchdog");
    __ASSERT(fs.init_storage() == FILE_SYSTEM_ERROR::STORAGE_OK, "Error to init storage");
    net.Attach(&sensor);
    net.Attach(&app);
    net.start();

    app.init_application();
    sensor.init();

    while (true)
    {
        k_sleep(K_FOREVER);
    }

    return 0;
}
