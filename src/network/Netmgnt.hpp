#pragma once
#include "IMQTT.hpp"
#include "IWifi.hpp"
#include "IStorage.hpp"
#include "ILed.hpp"
#include "IDispatcher.hpp"
#include <zephyr/kernel.h>

#define MAX_LISTERNERS 5



/*

state-flow

    [*] --> INITIALIZING : START
    INITIALIZING --> LOADING_CREDENTIALS: WIFI_IFACE_UP
    INITIALIZING --> IFACE_ERROR: WIFI_IFACE_ERROR

    LOADING_CREDENTIALS --> CONNECTING: WIFI_CREDS_OK
    LOADING_CREDENTIALS --> LOADING_CREDENTIALS: WIFI_CREDS_FAIL
    LOADING_CREDENTIALS --> ENABLING_AP: WIFI_CREDS_NOT_FOUND

    CONNECTING --> WAIT_IP: WIFI_CONNECTED

    CONNECTING --> CONNECTING: TIMEOUT && Tries < MAX
    CONNECTING --> ENABLING_AP: TIMEOUT && Tries > MAX
    
    WAIT_IP --> CONNECTED: IP_ACQUIRED
    WAIT_IP --> WAIT_IP: TIMEOUT && Tries < MAX
    WAIT_IP --> ENABLING_AP: TIMEOUT && Tries > MAX

    

    CONNECTED --> CONNECTING: WIFI_DISCONNECTED
    CONNECTED --> [*]

*/

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
    INITIALIZING,
    LOADING_CREDENTIALS,
    CONNECTING,
    WAIT_IP,
    CONNECTED,
    IFACE_ERROR,
    ENABLING_AP
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
    uint8_t tries;
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


private:
    void indicate_error(NET_ERROR err);
    static void rssi_monitor(struct k_work *work);
    NET_ERROR fail(NET_ERROR code, const char *msg);
    int32_t init_rssi_monitor();
    NET_ERROR set_wifi_credentials();

    void start_dhcp();
    void stop_dhcp();
    void connect_to_wifi();
    void start_mqtt();
    static void network_evt_dispatch_task(void *p1, void *, void *);
    void process_state(Events evt);
    void transition(WifiSmState new_state);
    void on_entry(WifiSmState state);

public:
    void Attach(IListener *listener);
    void Notify(Events evt);
    Netmgnt(IMQTT &mqtt, IWifi &wifi, IStorage &fs, ILed &led);
    NET_ERROR start();
    void stop();
    ~Netmgnt();
};
