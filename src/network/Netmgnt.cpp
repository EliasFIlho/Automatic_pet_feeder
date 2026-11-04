#include "Netmgnt.hpp"
#include "NetEvents.hpp"
#include <zephyr/logging/log.h>
#include "enum_to_string.hpp"
#include "types.hpp"
#include <zephyr/sys/reboot.h>
#include <zephyr/net/dns_sd.h>

LOG_MODULE_REGISTER(NETWORK_LOGS);

K_THREAD_STACK_DEFINE(NETWORK_DISPATCH_STACK_AREA, CONFIG_NETWORK_DISPATCH_THREAD_STACK_SIZE);

DNS_SD_REGISTER_TCP_SERVICE(petfeeder_config, CONFIG_NET_HOSTNAME,
                            "_petfeeder", "local", DNS_SD_EMPTY_TXT,
                            80);

Netmgnt::Netmgnt(IMQTT &mqtt, IWifi &wifi, ILed &led, IProvisioning &prov) : _mqtt(mqtt), _wifi(wifi), _led(led), _prov(prov)
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
        if (!self->_mqtt.is_connected())
        {
            self->_led.set_mapped_output(rssi, COLOR::GREEN, CONFIG_RSSI_LOWER_VALUE, CONFIG_RSSI_HIGHER_VALUE);
        }
        else
        {
            self->_led.set_output(COLOR::MAGENTA, 255);
        }
        k_work_reschedule(dwork, K_SECONDS(CONFIG_RSSI_WORK_PERIOD));
    }
}

void Netmgnt::check_wifi_credentials()
{

    this->_wifi.check_credentials();
}

void Netmgnt::scan_ssid_list()
{

    this->_wifi.scan_ssid_list();
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

void Netmgnt::Attach(IListener *listener, uint8_t evt_group_maks)
{
    if (this->listener_count < CONFIG_NETWORK_EVENT_MAX_LISTENERS)
    {
        this->listeners[this->listener_count].listener = listener;
        this->listeners[this->listener_count].evt_group = evt_group_maks;
        this->listener_count++;
        LOG_INF("Added Listener - %d", this->listener_count);
    }
    else
    {
        LOG_ERR("ERROR: MAX LISTENERS REACHED");
    }
}

void Netmgnt::Notify(EventMsg evt)
{
    for (int i = 0; i < this->listener_count; i++)
    {
        if (this->listeners[i].evt_group & evt.type)
        {
            this->listeners[i].listener->Update(evt.evt, evt.data);
        }
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
            if (evt.type == WIFI_EVT || evt.type == PROV_EVT)
            {
                LOG_WRN("ARRIVED EVENT: %s", EVENT_TO_STRING(evt.evt));
                self->process_state(evt.evt);
            }
            self->Notify(evt);
        }
    }
}

void Netmgnt::on_exit(WifiSmState from_state, WifiSmState to_state)
{
    LOG_INF("Leanving State %s", STATE_TO_STRING(from_state));
    switch (from_state)
    {
    case WifiSmState::INITIALIZING:
        // Nothing to do
        break;

    case WifiSmState::LOADING_CREDENTIALS:
        // Nothing to do
        break;

    case WifiSmState::CONNECTING:
        if (to_state != WifiSmState::CONNECTING)
        {
            this->wifi_sm.tries = 0;
        }
        break;

    case WifiSmState::WAITING_FOR_IP_ADDRESS:
        if (to_state != WifiSmState::WAITING_FOR_IP_ADDRESS)
        {
            this->wifi_sm.tries = 0;
        }
        break;

    case WifiSmState::CONNECTED:
        this->_mqtt.block_mqtt();
        this->_led.set_output(COLOR::RED, 255);
        k_work_cancel_delayable(&this->rssi_monitor_work);

        break;
    case WifiSmState::PROVISIONING:
        break;

    case WifiSmState::STORING_CRDENTIALS:
        break;

    case WifiSmState::RESTART:
        // Do nothing - device will be restarted, never reach herer
        break;
    default:
        break;
    }
}

void Netmgnt::transition(WifiSmState new_state)
{
    LOG_INF("From %s State -> %s State", STATE_TO_STRING(wifi_sm.state), STATE_TO_STRING(new_state));

    this->on_exit(this->wifi_sm.state, new_state);

    this->wifi_sm.state = new_state;

    this->on_entry(this->wifi_sm.state);
}

void Netmgnt::on_entry(WifiSmState state)
{
    switch (state)
    {
    case WifiSmState::INITIALIZING:
        this->set_led_output(COLOR::RED, 255);
        this->wifi_sm.tries = 0;
        this->_wifi.wifi_init();
        // this->scan_ssid_list(); // Maybe not needed
        break;

    case WifiSmState::LOADING_CREDENTIALS:
        this->check_wifi_credentials();
        break;

    case WifiSmState::CONNECTING:
        if (this->wifi_sm.tries > 0)
        {
            this->_wifi.wifi_disconnect();
        }
        k_sleep(K_MSEC(1000)); // Full second wait to wifi driver be able to get in STA mode
        this->connect_to_wifi();
        break;

    case WifiSmState::WAITING_FOR_IP_ADDRESS:
        this->set_led_output(COLOR::YELLOW, 255);
        this->start_dhcp();
        break;

    case WifiSmState::CONNECTED:
        k_msleep(100);
        this->init_rssi_monitor();
        this->_mqtt.release_mqtt();
        this->start_mqtt();
        if (this->_prov.isProvisioningEnabled_())
        {
            this->_prov.notify_connection_status(1);
            this->_prov.stop_provisioning();
        }
        break;
    case WifiSmState::PROVISIONING:
        this->set_led_output(COLOR::BLUE, 255);
        if (!this->_prov.isProvisioningEnabled_())
        {
            this->_prov.start_provisioning();
        }
        break;

    case WifiSmState::STORING_CRDENTIALS:
    //TODO: Create status code and verify the best place to call this
        this->_prov.notify_connection_status(250);
        this->_prov.write_credentials();
        break;
    case WifiSmState::IFACE_ERROR:
        LOG_ERR("WiFi interface error — cold rebooting");
        sys_reboot(SYS_REBOOT_COLD);
        break;
    case WifiSmState::RESTART:
        LOG_WRN("RESTARTING DEVICE");
        sys_reboot(SYS_REBOOT_COLD);
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
            this->on_entry(WifiSmState::INITIALIZING); // explicit first boot kick
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

            this->restart_state();
        }
        else if (evt == Events::WIFI_CREDS_NOT_FOUND)
        {

            this->transition(WifiSmState::PROVISIONING);
        }
        break;

    case WifiSmState::CONNECTING:
        if (evt == Events::WIFI_CONNECTED)
        {
            this->transition(WifiSmState::WAITING_FOR_IP_ADDRESS);
        }
        else if (evt == Events::TIMEOUT)
        {
            if (this->canRetry())
            {

                this->restart_state();
            }
            else
            {

                this->transition(WifiSmState::PROVISIONING);
            }
        }
        else if (evt == Events::WIFI_DISCONNECTED)
        {
            LOG_WRN("Ignoring WIFI_DISCONNECTED during CONNECTING");
        }
        break;
    case WifiSmState::WAITING_FOR_IP_ADDRESS:
        if (evt == Events::WIFI_IP_ACQUIRED)
        {
            this->transition(WifiSmState::CONNECTED);
        }
        else if (evt == Events::TIMEOUT)
        {
            if (this->canRetry())
            {

                this->restart_state();
            }
            else
            {

                this->transition(WifiSmState::PROVISIONING);
            }
        }
        else if (evt == Events::WIFI_DISCONNECTED)
        {
            this->_wifi.stop_connect_timer();
            this->transition(WifiSmState::CONNECTING);
        }
        break;
    case WifiSmState::CONNECTED:
        if (evt == Events::WIFI_DISCONNECTED)
        {
            this->transition(WifiSmState::CONNECTING);
        }
        break;
    case WifiSmState::PROVISIONING:
        if (evt == Events::PROVISIONING_CREDS_RECEIVED)
        {
            this->transition(WifiSmState::STORING_CRDENTIALS);
        }
        else if (evt == Events::PROVISIONING_STORE_CREDENTIALS_ERROR)
        {
            this->restart_state();
        }
        break;
    case WifiSmState::STORING_CRDENTIALS:
        if (evt == Events::PROVISIONING_STORE_CREDENTIALS_OK)
        {
            this->transition(WifiSmState::LOADING_CREDENTIALS);
        }
        else if (evt == Events::PROVISIONING_STORE_CREDENTIALS_ERROR)
        {
            this->transition(WifiSmState::PROVISIONING);
        }
        break;
    case WifiSmState::IFACE_ERROR:
        // Never reached
        break;

    case WifiSmState::RESTART:
        // Never reached
        break;
    default:
        break;
    }
}

void Netmgnt::set_led_output(COLOR color, uint8_t brightness)
{
    this->_led.set_output(color, brightness);
}

void Netmgnt::restart_state()

{

    LOG_INF("Restarting state %s", STATE_TO_STRING(wifi_sm.state));

    this->on_exit(this->wifi_sm.state, this->wifi_sm.state);

    this->on_entry(this->wifi_sm.state);
}

bool Netmgnt::canRetry()
{
    return ++this->wifi_sm.tries < CONFIG_NETWORK_CONNECTION_MAX_TRIES;
}

void Netmgnt::notify_evt(Events evt, void *data)
{
    EventMsg msg{.evt = evt,
                 .type = WIFI_EVT};

    k_msgq_put(&net_evt_queue, &msg, K_NO_WAIT);
}

// TODO: Update state machine to use a new provisioning interface instead.