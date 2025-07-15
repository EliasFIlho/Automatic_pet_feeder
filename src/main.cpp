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
#include "Storage.hpp"

#define WIFI_SSID "LINKCE- 2G"
#define WIFI_PSK "20122000"

WifiStation network;
// WifiSoftAp Sap;

HttpsClient client;

Storage fs;

// TODO: Create the storage module using ZMS for, make the module works as a Sigleton for use control

int main(void)
{

    char ssid[16];

    k_sleep(K_SECONDS(3));
    int ret = fs.init_storage();

    if(ret != 0){
        printk("Error to init Fs\r\n");
    }

    ret = fs.read_data(SSID_ID,ssid,sizeof(ssid));
    
    if(ret < 0){
        printk("No data stored\r\n");
        ret = fs.write_data(SSID_ID,WIFI_SSID);
        if(ret < 0){
            printk("Error to write SSID data\r\n");
        }
    }else{
        printk("Data founded in FS\r\n");
        printk("SSID data: [%s]\r\n",ssid);
    }

    printk("Initing wifi...\r\n");
    network.wifi_init();
    network.connect_to_wifi(ssid, WIFI_PSK);
    ret = client.setup_socket();
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
