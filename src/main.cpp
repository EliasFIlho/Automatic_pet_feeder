/*
 *
 * Init Wifi and send a HTTP GET request to google
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "Application.hpp"
#include <zephyr/task_wdt/task_wdt.h>

#define WIFI_SSID ""
#define WIFI_PSK ""

#define RE_WRITE 0

/*Threads:

1.  Check rules and control actuator - Application task
2.  Handle incoming rules(commands also?) - MQTT task ------
                                                            |- Both in the MQTT module? 
                                                            |-(Maybe configure MQTT to act as a singleton and get a instance in different threads like Storage)
3.  Handle publish data - MQTT_Publish task ----------------
4.  Check food dispener level - Lvl Sensor task (This task needs to comunicate with MQTT_Publish task, maybe a queue with a struct publish_payload)

*/

Application app;





int main(void)
{
 
#if RE_WRITE
    int ret = fs.write_data(SSID_ID, WIFI_SSID);
    if (ret < 0)
    {
        printk("Error to write SSID data\r\n");
    }

    ret = fs.write_data(PASSWORD_ID, WIFI_PSK);
    if (ret < 0)
    {
        printk("Error to write SSID data\r\n");
    }

#endif
    //TODO:CHANGE ALL THE printk CALLS TO LOG MODULE (That way i can get more detailed logs about execution and probably easer to disable logs later) 
    //Note: For some reason i cant do that probably something about log buffer size, not quite sure about this. (I'll try things like increase thread stack size or log buffer in proj.cof)
    
    //TODO: Add hardware watchdog as fallback for task watchdog
    int ret = task_wdt_init(NULL);

    if(ret != 0){
        printk("Error to init task watchdog\n\r");
    }else{
        printk("Watchdog task inited\n\r");
    }

    app.start_application();
    while (1)
    {
        k_sleep(K_FOREVER);
    }
        
    return 0;
}
