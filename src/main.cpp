/*
 *
 * Init Wifi and send a HTTP GET request to google
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "WifiStation.hpp"
#include "StepperController.hpp"
#include "SntpClient.hpp"
#include "HttpsClient.hpp"

#define WIFI_SSID "LINKCE- 2G"
#define WIFI_PSK "20122000"

WifiStation network;
StepperController motor;
SntpClient sntp;
HttpsClient client;

int main(void)
{

    k_sleep(K_SECONDS(3));
    printk("Initing wifi...\r\n");
    network.wifi_init();
    network.connect_to_wifi(WIFI_SSID, WIFI_PSK);

    
    int ret = client.setup_socket();
    if (ret != 0)
    {
        printk("Error to setup_socket - file number [%d]\r\n", ret);
        network.wifi_disconnect();
    }
    k_msleep(1000);
    client.connect_socket();
    k_msleep(1000);
    client.get_package();

    // while (1)
    // {
    //     sntp.update_current_time();
    //     motor.move_for(400);

    // }

    network.wifi_disconnect();

    return 0;
}
