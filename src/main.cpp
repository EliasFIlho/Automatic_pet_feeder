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
#include "Watchdog.hpp"
#include <zephyr/task_wdt/task_wdt.h>



// TODO: Use main as a wiring point to start tasks (Avoid nested threads)
int main(void)
{
    WifiStation wifi;
    MQTT mqtt;
    StepperController motor;
    RTC rtc;
    Storage fs;
    Watchdog guard;
    NetworkService net(mqtt, wifi, fs);
    Application app(rtc, motor, fs, guard);

    guard.init();

    int ret = fs.init_storage();
    if (ret != 0)
    {
        printk("Error to init Fs\r\n");
        return -1;
    }

    

    

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
