#pragma once
#include "IMQTT.hpp"
#include "IWifi.hpp"
#include "IStorage.hpp"
#include "ILed.hpp"
#include "IDispatcher.hpp"
#include <zephyr/kernel.h>

#define MAX_LISTERNERS 5

enum class NET_ERROR : uint8_t
{
    NET_OK,
    MISSING_WIFI_CREDENTIALS,
    WIFI_INIT_ERROR,
    WIFI_TIMEOUT,
    IFACE_MISSING
};

enum class WifiSmState
{
    INIT,
    LOADING_CREDENTIALS,
    CONNECTING,
    WAIT_IP,
    CONNECTED,
    ERROR
};

struct state_transition
{
    WifiSmState from;
    Events evt;
    WifiSmState to;
};

struct Wifi_State_Machine
{

    WifiSmState state;
};

class Netmgnt : public IDispatcher
{
private:
    IMQTT &_mqtt;
    IWifi &_wifi;
    IStorage &_fs;
    ILed &_led;
    struct k_work_delayable rssi_monitor_work;
    struct k_thread dispatcher_thread;
    uint8_t listener_count = 0;
    IListener *listeners[MAX_LISTERNERS];

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

    struct Wifi_State_Machine wifi_sm;

    inline static constexpr state_transition transitions_tbl[] = {
        // INIT
        {WifiSmState::INIT, Events::WIFI_IFACE_UP, WifiSmState::LOADING_CREDENTIALS},
        {WifiSmState::INIT, Events::WIFI_IFACE_ERROR, WifiSmState::ERROR},

        // LOADING_CREDENTIALS
        {WifiSmState::LOADING_CREDENTIALS, Events::WIFI_CREDS_OK, WifiSmState::CONNECTING},
        {WifiSmState::LOADING_CREDENTIALS, Events::WIFI_CREDS_FAIL, WifiSmState::ERROR},

        // CONNECTING
        {WifiSmState::CONNECTING, Events::WIFI_CONNECTED, WifiSmState::WAIT_IP},
        {WifiSmState::CONNECTING, Events::TIMEOUT, WifiSmState::ERROR},

        // WAIT_IP
        {WifiSmState::WAIT_IP, Events::IP_ACQUIRED, WifiSmState::CONNECTED},
        {WifiSmState::WAIT_IP, Events::TIMEOUT, WifiSmState::ERROR},

        // CONNECTED
        {WifiSmState::CONNECTED, Events::WIFI_DISCONNECTED, WifiSmState::CONNECTING},

        // ERROR
        {WifiSmState::ERROR, Events::RETRY, WifiSmState::LOADING_CREDENTIALS}};

private:
    void indicate_error(NET_ERROR err);
    static void rssi_monitor(struct k_work *work);
    NET_ERROR fail(NET_ERROR code, const char *msg);
    void process_evt(Events evt);
    void state_enter(WifiSmState state);
    int32_t init_rssi_monitor();
    NET_ERROR set_wifi_credentials();

    void start_dhcp();
    void stop_dhcp();
    void connect_to_wifi();
    void start_mqtt();
    static void network_evt_dispatch_task(void *p1, void *, void *);

public:
    void Attach(IListener *listener);
    void Notify(Events evt);
    Netmgnt(IMQTT &mqtt, IWifi &wifi, IStorage &fs, ILed &led);
    NET_ERROR start();
    void stop();
    ~Netmgnt();
};
