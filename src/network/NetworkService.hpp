#pragma once
#include "IMQTT.hpp"
#include "IWifi.hpp"
#include "IStorage.hpp"
#include "ILed.hpp"
#include <zephyr/kernel.h>

enum class NET_ERROR : uint8_t
{
    NET_OK,
    MISSING_WIFI_CREDENTIALS,
    WIFI_INIT_ERROR,
    WIFI_TIMEOUT,
    IFACE_MISSING
};

class NetworkService
{
private:
    IMQTT &_mqtt;
    IWifi &_wifi;
    IStorage &_fs;
    ILed &_led;
    struct k_work_delayable rssi_monitor_work;

    struct BlinkPattern
    {
        uint16_t on_time_ms;
        uint16_t off_time_ms;
        uint8_t repeat;
    };

    // TODO: Move those values to Kconfig to avoid magic numbers
    inline static constexpr BlinkPattern error_blink_table[] = {
        {0, 0, 0},
        {200, 200, 2},
        {500, 500, 3},
        {1000, 1000, 2},
        {100, 900, 5},
    };

private:
    void indicate_error(NET_ERROR err);
    static void rssi_monitor(struct k_work *work);

public:
    NetworkService(IMQTT &mqtt, IWifi &wifi, IStorage &fs, ILed &led);
    bool is_mqtt_updated_payload();
    int32_t init_rssi_monitor();
    NET_ERROR start();
    void stop();
    ~NetworkService();
};
