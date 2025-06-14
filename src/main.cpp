/*
 *
 * Init Wifi and send a HTTP GET request to google
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "wifi_station.hpp"
#include "sntp.hpp"
#include "stepper.hpp"



/* STA Mode Configuration */

// This is a temp solution until I made the AP mode to receive the credentials
#define WIFI_SSID ""
#define WIFI_PSK ""

stepper motor;
Wifi_Station network;
sntp ntp_obj;


//Basic application to see if everything is working as should.

//TODO: Start the real application ;p
int main(void)
{

    k_sleep(K_SECONDS(3));
    printk("Initing wifi...\r\n");
    network.wifi_init();
    network.connect_to_wifi(WIFI_SSID, WIFI_PSK);
   
    while (1)
    {
        ntp_obj.get_current_time();
        motor.move_for(100);
        k_msleep(3000);

    }

    network.wifi_disconnect();

    return 0;
}
