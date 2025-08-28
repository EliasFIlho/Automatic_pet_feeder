/*
 *
 * Init Wifi and send a HTTP GET request to google
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "Application.hpp"

#define WIFI_SSID ""
#define WIFI_PSK ""

#define RE_WRITE 0

Application app;

int main(void)
{
 
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

#endif
    //TODO:CHANGE ALL THE printk CALLS TO LOG MODULE (That way i can get more detailed logs about execution and probably easer to disable logs later)
    app.start_application();
    while (1)
    {
        k_msleep(5000);
    }
        
    return 0;
}
