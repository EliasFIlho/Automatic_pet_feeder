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
#include "RTC.hpp"

#define WIFI_SSID ""
#define WIFI_PSK ""

#define RE_WRITE 0

WifiStation network;
HttpsClient client;
RTC rtc;


int main(void)
{
    int ret;
    char ssid[16];
    char psk[16];
    Storage& fs = Storage::getInstance();
    k_sleep(K_SECONDS(3));
    ret = fs.init_storage();

    if (ret != 0)
    {
        printk("Error to init Fs\r\n");
    }

#if RE_WRITE
    ret = fs.write_data(SSID_ID, WIFI_SSID);
    if (ret < 0)
    {
        printk("Error to write SSID data\r\n");
    }

    ret = fs.write_data(PASSWORD_ID, WIFI_PSK);
    if (ret < 0)
    {
        printk("Error to write SSID data\r\n");
    }
#else
    ret = fs.read_data(SSID_ID, ssid, sizeof(ssid));

    if (ret < 0)
    {
        printk("No data stored\r\n");
        ret = fs.write_data(SSID_ID, WIFI_SSID);
        if (ret < 0)
        {
            printk("Error to write SSID data\r\n");
        }
    }
    else
    {
        printk("Data founded in FS\r\n");
        printk("SSID data: [%s]\r\n", ssid);
    }

    ret = fs.read_data(PASSWORD_ID, psk, sizeof(psk));
    if (ret < 0)
    {
        printk("No data stored\r\n");
        ret = fs.write_data(PASSWORD_ID, WIFI_PSK);
        if (ret < 0)
        {
            printk("Error to write SSID data\r\n");
        }
    }
    else
    {
        printk("Data founded in FS\r\n");
        printk("PSK data: [%s]\r\n", psk);
    }

#endif
    printk("Initing wifi...\r\n");
    network.wifi_init();
    network.connect_to_wifi(ssid, psk);
    
    
    ret = client.setup_socket();
    if (ret != 0)
    {
        printk("Error to setup_socket - file number [%d]\r\n", ret);
        network.wifi_disconnect();
        return 0;
    }
    client.connect_socket();
    client.get_package();
    
    
    rtc.sync_time();
    int week = rtc.get_week_day();
    printk("Week day [%d]\n\r",week);
    
    network.wifi_disconnect();

    return 0;
}
