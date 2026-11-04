#pragma once
#include "IMQTT.hpp"
#include "IWifi.hpp"
#include "ILed.hpp"
#include "IDispatcher.hpp"
#include "IHTTPServer.hpp"
#include "IProvisioning.hpp"
#include <zephyr/kernel.h>
#include <array>
#include "types.hpp"


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
    ILed &_led;
    IProvisioning &_prov;

    struct k_work_delayable rssi_monitor_work;
    struct k_thread dispatcher_thread;
    uint8_t listener_count = 0;
    std::array<Listeners, CONFIG_NETWORK_EVENT_MAX_LISTENERS> listeners {};
    struct Wifi_State_Machine wifi_sm;

private:
    static void rssi_monitor(struct k_work *work);
    int32_t init_rssi_monitor();

    void check_wifi_credentials();

    void start_dhcp();
    void stop_dhcp();
    void connect_to_wifi();
    void scan_ssid_list();
    void start_mqtt();
    static void network_evt_dispatch_task(void *p1, void *, void *);
    void set_led_output(COLOR color, uint8_t brightness);
    void process_state(Events evt);
    void transition(WifiSmState new_state);
    void on_entry(WifiSmState state);
    void on_exit(WifiSmState from_state, WifiSmState to_state);
    void restart_state();
    bool canRetry();
    void notify_evt(Events evt, void * data);
public:
    void Attach(IListener *listener, uint8_t evt_group_maks);
    void Notify(EventMsg evt);
    Netmgnt(IMQTT &mqtt, IWifi &wifi, ILed &led, IProvisioning &prov);
    void start();
    void stop();
    ~Netmgnt();
};
