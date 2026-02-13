#include "Netmgnt.hpp"
#include "NetEvents.hpp"
#include <zephyr/logging/log.h>
#include "enum_to_string.hpp"

/*
TODO: Create private methods for each operation that calls other objects methods
    e.g. "this->_http.start();" becomes "this->start_http_server();"
*/

LOG_MODULE_REGISTER(NETWORK_LOGS);

K_THREAD_STACK_DEFINE(NETWORK_DISPATCH_STACK_AREA, CONFIG_NETWORK_DISPATCH_THREAD_STACK_SIZE);

Netmgnt::Netmgnt(IMQTT &mqtt, IWifi &wifi, ILed &led, IWifiAp &soft_ap, IHTTPServer &http_server) : _mqtt(mqtt), _wifi(wifi), _led(led), _ap(soft_ap), _http(http_server)
{
}

Netmgnt::~Netmgnt()
{
}

void Netmgnt::start()
{
    k_thread_create(&this->dispatcher_thread,
                    NETWORK_DISPATCH_STACK_AREA,
                    CONFIG_NETWORK_DISPATCH_THREAD_STACK_SIZE,
                    Netmgnt::network_evt_dispatch_task,
                    this, NULL, NULL, CONFIG_NETWORK_DISPATCH_THREAD_PRIORITY, 0, K_NO_WAIT);
}

void Netmgnt::stop()
{
    this->_mqtt.abort();
    this->_wifi.wifi_disconnect();
}

int32_t Netmgnt::init_rssi_monitor()
{
    int ret = this->_led.init();
    if (ret != 0)
    {
        return -EIO;
    }
    else
    {
        k_work_init_delayable(&this->rssi_monitor_work, this->rssi_monitor);
        k_work_reschedule(&this->rssi_monitor_work, K_NO_WAIT);
        return 0;
    }
}

void Netmgnt::rssi_monitor(struct k_work *work)
{
    k_work_delayable *dwork = k_work_delayable_from_work(work);
    auto *self = CONTAINER_OF(dwork, Netmgnt, rssi_monitor_work);
    if (self->wifi_sm.state == WifiSmState::CONNECTED)
    {
        int32_t rssi = self->_wifi.get_rssi();
        LOG_WRN("RSSI VALUE: %d", rssi);
        self->_led.set_mapped_output(rssi, COLOR::GREEN, CONFIG_RSSI_LOWER_VALUE, CONFIG_RSSI_HIGHER_VALUE);
        k_work_reschedule(dwork, K_SECONDS(CONFIG_RSSI_WORK_PERIOD));
    }
}

void Netmgnt::set_wifi_credentials()
{

    this->_wifi.set_credentials();
}

void Netmgnt::connect_to_wifi()
{
    this->_wifi.connect_to_wifi();
}

void Netmgnt::start_dhcp()
{
    this->_wifi.start_dhcp();
}

void Netmgnt::stop_dhcp()
{
}

void Netmgnt::start_mqtt()
{
    this->_mqtt.start_mqtt();
}

void Netmgnt::Attach(IListener *listener)
{
    if (this->listener_count < MAX_LISTERNERS)
    {
        this->listeners[this->listener_count] = listener;
        this->listener_count++;
        LOG_INF("Added Listener - %d", this->listener_count);
    }
    else
    {
        LOG_ERR("ERROR: MAX LISTENERS REACHED");
    }
}

void Netmgnt::Notify(Events evt)
{
    for (int i = 0; i < this->listener_count; i++)
    {
        this->listeners[i]->Update(evt);
    }
}

void Netmgnt::network_evt_dispatch_task(void *p1, void *, void *)
{
    auto *self = static_cast<Netmgnt *>(p1);
    EventMsg evt = {};
    self->wifi_sm.state = WifiSmState::INITIALIZING;
    self->process_state(Events::START);

    while (true)
    {
        if (k_msgq_get(&net_evt_queue, &evt, K_FOREVER) == 0)
        {
            // Only process WIFI events types
            if (evt.type == EventGroup::WIFI)
            {
                LOG_WRN("ARRIVED EVENT: %s", EVENT_TO_STRING(evt.evt));
                self->process_state(evt.evt);
            }
            self->Notify(evt.evt);
        }
    }
}

//TODO: Maybe add the next state as parameter to open the range of logics (WifiSmState state, WifiSmState new_state)
void Netmgnt::on_exit(WifiSmState state)
{
    LOG_INF("Leanving State %s", STATE_TO_STRING(state));
    switch (state)
    {
    case WifiSmState::INITIALIZING:
        //TODO: Check if is something to before exit INITIALIZING state
        break;

    case WifiSmState::LOADING_CREDENTIALS:
        //TODO: Check if is something to before exit LOADING_CREDENTIALS state
        break;

    case WifiSmState::CONNECTING:
        //TODO: Check if is something to before exit CONNECTING state
        break;

    case WifiSmState::WAIT_IP:
        //TODO: Check if is something to before exit WAIT_IP state
        break;

    case WifiSmState::CONNECTED:
        //TODO: Check if is something to before exit CONNECTED state
        //TODO: Block MQTT and Stop RSSI monitor
        break;
    case WifiSmState::ENABLING_AP:
        //TODO: Check if is something to before exit ENABLING_AP state
        break;
    case WifiSmState::ENABLING_HTTP_SERVER:
        //TODO: Check if is something to before exit ENABLING_HTTP_SERVER state
        //TODO: Disable AP before and stop http server before leave this state
    default:
        break;
    }
}

void Netmgnt::transition(WifiSmState new_state)
{
    LOG_INF("From %s State -> %s State", STATE_TO_STRING(wifi_sm.state), STATE_TO_STRING(new_state));

    this->on_exit(this->wifi_sm.state);

    this->wifi_sm.state = new_state;

    this->on_entry(this->wifi_sm.state);
}

void Netmgnt::on_entry(WifiSmState state)
{
    switch (state)
    {
    case WifiSmState::INITIALIZING:
        this->set_led_output(COLOR::RED, 255);
        this->_wifi.wifi_init();
        break;

    case WifiSmState::LOADING_CREDENTIALS:
        this->set_wifi_credentials();
        break;

    case WifiSmState::CONNECTING:
        this->_mqtt.block_mqtt();
        k_work_cancel_delayable(&this->rssi_monitor_work);
        this->connect_to_wifi();
        break;

    case WifiSmState::WAIT_IP:
        this->set_led_output(COLOR::YELLOW, 255);
        this->start_dhcp();
        break;

    case WifiSmState::CONNECTED:
        this->wifi_sm.tries = 0;
        this->init_rssi_monitor();
        // this->_mqtt.release_mqtt();
        this->start_mqtt();
        break;
    case WifiSmState::ENABLING_AP:
        this->wifi_sm.tries = 0;
        this->set_led_output(COLOR::BLUE, 255);
        this->_ap.ap_init();
        break;
    case WifiSmState::ENABLING_HTTP_SERVER:
        this->_http.start();
        break;
    default:
        break;
    }
}

void Netmgnt::process_state(Events evt)
{

    switch (this->wifi_sm.state)
    {
    case WifiSmState::INITIALIZING:
        if (evt == Events::START)
        {
            this->transition(WifiSmState::INITIALIZING);
        }
        else if (evt == Events::WIFI_IFACE_UP)
        {
            this->transition(WifiSmState::LOADING_CREDENTIALS);
        }
        else if (evt == Events::WIFI_IFACE_ERROR)
        {
            this->transition(WifiSmState::IFACE_ERROR);
        }
        break;
    case WifiSmState::LOADING_CREDENTIALS:
        if (evt == Events::WIFI_CREDS_OK)
        {

            this->transition(WifiSmState::CONNECTING);
        }
        else if (evt == Events::WIFI_CREDS_FAIL)
        {

            this->transition(WifiSmState::LOADING_CREDENTIALS);
        }
        else if (evt == Events::WIFI_CREDS_NOT_FOUND)
        {

            this->transition(WifiSmState::ENABLING_AP);
        }
        break;
    
    case WifiSmState::CONNECTING:
        if (evt == Events::WIFI_CONNECTED)
        {
            this->transition(WifiSmState::WAIT_IP);
        }
        else if (evt == Events::TIMEOUT)
        {
            if (++wifi_sm.tries < CONFIG_NETWORK_CONNECTION_MAX_TRIES)
            {

                this->transition(WifiSmState::CONNECTING);
            }
            else
            {

                this->transition(WifiSmState::ENABLING_AP);
            }
        }else if(evt == Events::WIFI_DISCONNECTED){
            // TODO: For some reason the wifi driver sometimes (specially after reset) rise the WIFI_DISCONNECTED event, i need to see a good way to handle this.
            LOG_WRN("See how to handle wifi disconnect event during the connect request, but right now, just try again");
            this->_wifi.stop_connect_timer();
            this->transition(WifiSmState::CONNECTING);
        }
        break;
    case WifiSmState::WAIT_IP:
        if (evt == Events::WIFI_IP_ACQUIRED)
        {
            this->transition(WifiSmState::CONNECTED);
        }
        else if (evt == Events::TIMEOUT)
        {
            if (++wifi_sm.tries < CONFIG_NETWORK_CONNECTION_MAX_TRIES)
            {

                this->transition(WifiSmState::WAIT_IP);
            }
            else
            {

                this->transition(WifiSmState::ENABLING_AP);
            }
        }else if(evt == Events::WIFI_DISCONNECTED){
            this->_wifi.stop_connect_timer();
            LOG_WRN("See how to handle wifi disconnect event during the connect request, but right now, just try again");
            this->transition(WifiSmState::CONNECTING);
        }
        break;
    case WifiSmState::CONNECTED:
        if (evt == Events::WIFI_DISCONNECTED)
        {
            this->transition(WifiSmState::CONNECTING);
        }
        break;
    case WifiSmState::IFACE_ERROR:
        break;
    case WifiSmState::ENABLING_AP:
        if (evt == Events::WIFI_AP_ENABLE)
        {
            this->transition(WifiSmState::ENABLING_HTTP_SERVER);
        }
        break;
    
    case WifiSmState::ENABLING_HTTP_SERVER: //Check the name for this state
        if(evt == Events::HTTP_STORED_CREDENTIALS){
            this->transition(WifiSmState::LOADING_CREDENTIALS);
        }
        break;
    default:
        break;
    }
}

void Netmgnt::set_led_output(COLOR color, uint8_t brightness)
{
    this->_led.set_output(color, 255);
}