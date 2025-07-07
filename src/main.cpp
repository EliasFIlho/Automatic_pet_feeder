/*
 *
 * Init Wifi and send a HTTP GET request to google
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "WifiStation.hpp"

#include "HttpsClient.hpp"
#include "WifiSoftAp.hpp"

#define WIFI_SSID ""
#define WIFI_PSK ""

WifiStation network;
//WifiSoftAp Sap;

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
        return 0;
    }
    client.connect_socket();
    client.get_package();
    
    network.wifi_disconnect();

    return 0;
}
