#include "Netmgnt.hpp"
#include "NetEvents.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(NETWORK_LOGS);
#define SSID_TEMP_BUFFER_LEN 16
#define PSK_TEMP_BUFFER_LEN 16

K_THREAD_STACK_DEFINE(NETWORK_DISPATCH_STACK_AREA, CONFIG_NETWORK_DISPATCH_THREAD_STACK_SIZE);

Netmgnt::Netmgnt(IMQTT &mqtt, IWifi &wifi, IStorage &fs, ILed &led) : _mqtt(mqtt), _wifi(wifi), _fs(fs), _led(led)
{
}

Netmgnt::~Netmgnt()
{
}

NET_ERROR Netmgnt::start()
{
    k_thread_create(&this->dispatcher_thread,
                    NETWORK_DISPATCH_STACK_AREA,
                    CONFIG_NETWORK_DISPATCH_THREAD_STACK_SIZE,
                    Netmgnt::network_evt_dispatch_task,
                    this, NULL, NULL, CONFIG_NETWORK_DISPATCH_THREAD_PRIORITY, 0, K_NO_WAIT);

    return NET_ERROR::NET_OK;
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

void Netmgnt::indicate_error(NET_ERROR err)
{
    const auto &pattern = error_blink_table[static_cast<uint8_t>(err)];
    this->_led.set_output(COLOR::RED, 0);

    for (int i = 0; i != pattern.repeat; i++)
    {
        this->_led.set_output(COLOR::RED, 255);
        k_msleep(pattern.on_time_ms);
        this->_led.set_output(COLOR::RED, 0);
        k_msleep(pattern.off_time_ms);
    }
}

void Netmgnt::rssi_monitor(struct k_work *work)
{
    k_work_delayable *dwork = k_work_delayable_from_work(work);
    auto *self = CONTAINER_OF(dwork, Netmgnt, rssi_monitor_work);
    if (self->wifi_sm.state == WifiSmState::CONNECTED)
    {
        int32_t rssi = self->_wifi.get_rssi();
        LOG_WRN("RSSI VALUE: %d",rssi);
        self->_led.set_mapped_output(rssi, COLOR::GREEN, CONFIG_RSSI_LOWER_VALUE, CONFIG_RSSI_HIGHER_VALUE);
    }
    else
    {
        self->_led.set_output(COLOR::RED, 255);
    }
    k_work_reschedule(dwork, K_SECONDS(CONFIG_RSSI_WORK_PERIOD));
}

NET_ERROR Netmgnt::fail(NET_ERROR code, const char *msg)
{
    LOG_ERR("%s", msg);
    this->indicate_error(code);
    return code;
}

NET_ERROR Netmgnt::set_wifi_credentials()
{
    char ssid[SSID_TEMP_BUFFER_LEN];
    char psk[PSK_TEMP_BUFFER_LEN];
    int ret;
    LOG_INF("Start Network LOGS");

    if (this->_fs.read_buffer(SSID_ID, ssid, sizeof(ssid)) < 0)
    {
        return this->fail(NET_ERROR::MISSING_WIFI_CREDENTIALS, "SSID Missing");
    }
    if (this->_fs.read_buffer(PASSWORD_ID, psk, sizeof(psk)) < 0)
    {
        return this->fail(NET_ERROR::MISSING_WIFI_CREDENTIALS, "Password Missing");
    }

    this->_wifi.set_credentials(ssid, psk);
    return NET_ERROR::NET_OK;
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
                self->process_state(evt.evt);
            }
            self->Notify(evt.evt);
        }
    }
}

void Netmgnt::transition(WifiSmState new_state)
{
    LOG_INF("State %d -> %d", static_cast<int>(wifi_sm.state), static_cast<int>(new_state));

    wifi_sm.state = new_state;

    this->on_entry(new_state);
}



void Netmgnt::on_entry(WifiSmState state)
{
    switch (state)
    {
    case WifiSmState::INITIALIZING:
        this->_led.set_output(COLOR::RED, 255);
        this->_wifi.wifi_init();
        break;

    case WifiSmState::LOADING_CREDENTIALS:
        this->set_wifi_credentials();
        break;

    case WifiSmState::CONNECTING:
        this->connect_to_wifi();
        break;

    case WifiSmState::WAIT_IP:
        this->_led.set_output(COLOR::YELLOW, 255);
        this->start_dhcp();
        break;

    case WifiSmState::CONNECTED:
        this->wifi_sm.tries = 0;
        this->init_rssi_monitor();
        this->_mqtt.release_mqtt();
        this->start_mqtt();

        break;
    case WifiSmState::ENABLING_AP:
        this->wifi_sm.tries = 0;
        LOG_ERR("will implement this soon");
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

            transition(WifiSmState::CONNECTING);
        }
        else if (evt == Events::WIFI_CREDS_FAIL)
        {

            transition(WifiSmState::LOADING_CREDENTIALS);
        }
        else if (evt == Events::WIFI_CREDS_NOT_FOUND)
        {

            transition(WifiSmState::ENABLING_AP);
        }
        break;
    case WifiSmState::CONNECTING:
        if (evt == Events::WIFI_CONNECTED)
        {
            transition(WifiSmState::WAIT_IP);
        }
        else if (evt == Events::TIMEOUT)
        {
            if (++wifi_sm.tries < CONFIG_NETWORK_CONNECTION_MAX_TRIES)
                transition(WifiSmState::CONNECTING);
            else
                transition(WifiSmState::ENABLING_AP);
        }
        break;
    case WifiSmState::WAIT_IP:
        if (evt == Events::IP_ACQUIRED)
        {
            transition(WifiSmState::CONNECTED);
        }
        else if (evt == Events::TIMEOUT)
        {
            if (++wifi_sm.tries < CONFIG_NETWORK_CONNECTION_MAX_TRIES)
                transition(WifiSmState::WAIT_IP);
            else
                transition(WifiSmState::ENABLING_AP);
        }
        break;
    case WifiSmState::CONNECTED:
        if (evt == Events::WIFI_DISCONNECTED)
        {
            transition(WifiSmState::CONNECTING);
        }
        break;
    case WifiSmState::IFACE_ERROR:
        
        break;
    case WifiSmState::ENABLING_AP:
        break;
    default:
        break;
    }
}
