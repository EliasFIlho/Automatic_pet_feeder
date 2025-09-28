#pragma once
#include "IMQTT.hpp"
#include "IWifi.hpp"
#include "IStorage.hpp"
#include "ILed.hpp"
#include <zephyr/kernel.h>



class NetworkService
{
private:
    IMQTT &_mqtt;
    IWifi &_wifi;
    IStorage &_fs;
    ILed &_led;
    struct k_work_delayable rssi_monitor_work;

private:
    static void rssi_monitor(struct k_work *work);

public:
    NetworkService(IMQTT &mqtt, IWifi &wifi, IStorage &fs,ILed &led);
    bool is_mqtt_updated_payload();
    int32_t init_rssi_monitor();
    bool start();
    void stop();
    ~NetworkService();
};
