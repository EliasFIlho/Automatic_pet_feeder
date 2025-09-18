/*
 *
 * Init Wifi and send a HTTP GET request to google
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "Application.hpp"
#include "StepperController.hpp"
#include "RTC.hpp"
#include "Storage.hpp"
#include "SchedulerRules.hpp"
#include "WifiStation.hpp"
#include "MQTT.hpp"
#include "NetworkService.hpp"
#include <zephyr/task_wdt/task_wdt.h>

// TODO: Refactor the modules to use interfaces

// TODO: Create a network module to merge wifi and mqtt in a higher level way

// TODO: Use main as a wiring point to start tasks (Avoid nested threads)
int main(void)
{
    WifiStation wifi;
    MQTT mqtt;
    StepperController motor;
    RTC rtc;
    Storage store;
    NetworkService net(mqtt, wifi, store);
    Application app(rtc, motor, store);

    int ret = store.init_storage();
    if (ret != 0)
    {
        printk("Error to init Fs\r\n");
        return -1;
    }

    ret = task_wdt_init(NULL);

    if (ret != 0)
    {
        printk("Error to init task watchdog\n\r");
    }
    else
    {
        printk("Watchdog task inited\n\r");
    }
    //TODO: CHECK WHY I CANT CONNECT TO THIS FCK WIFI ANYMORE
    if(net.start()){
        app.start_application();

    }else{
        printk("Error to start network\n\r");
    }
    while (true)
    {
        k_sleep(K_FOREVER);
    }

    return 0;
}
