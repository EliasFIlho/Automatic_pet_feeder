#include "Netmgnt.hpp"
#include "NetEvents.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(NETWORK_LOGS);
#define MAX_ATTEMPT 3
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
    NET_ERROR ret = this->connect_to_wifi();
    if (ret != NET_ERROR::NET_OK)
    {
        return ret;
    }
    this->start_mqtt();
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
    this->_led.set_output(LOW);

    for (int i = 0; i != pattern.repeat; i++)
    {
        this->_led.set_output(HIGH);
        k_msleep(pattern.on_time_ms);
        this->_led.set_output(LOW);
        k_msleep(pattern.off_time_ms);
    }
}

void Netmgnt::rssi_monitor(struct k_work *work)
{
    k_work_delayable *dwork = k_work_delayable_from_work(work);
    auto *self = CONTAINER_OF(dwork, Netmgnt, rssi_monitor_work);
    if (self->_wifi.is_connected())
    {
        int32_t rssi = self->_wifi.get_rssi();
        self->_led.set_mapped_output(rssi, CONFIG_RSSI_LOWER_VALUE, CONFIG_RSSI_HIGHER_VALUE);
    }
    else
    {
        self->_led.set_output(LOW);
    }
    k_work_reschedule(dwork, K_SECONDS(CONFIG_RSSI_WORK_PERIOD));
}

NET_ERROR Netmgnt::fail(NET_ERROR code, const char *msg)
{
    LOG_ERR("%s", msg);
    this->indicate_error(code);
    return code;
}

void Netmgnt::network_evt_dispatch_task(void *p1, void *, void *)
{
    {
        auto *self = static_cast<Netmgnt *>(p1);
        NetEventMsg evts;
        while (true)
        {
            if (k_msgq_get(&net_evt_queue, &evts, K_FOREVER) == 0)
            {
                LOG_WRN("Got event");
                self->Notify(evts.evt);
            }
        }
    }
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

    this->_wifi.set_wifi_ssid(ssid);
    this->_wifi.set_wifi_psk(psk);
    return NET_ERROR::NET_OK;
}

NET_ERROR Netmgnt::connect_to_wifi()
{
    int ret;

    if (!this->_wifi.wifi_init())
    {
        return this->fail(NET_ERROR::WIFI_INIT_ERROR, "Error to init wifi");
    }
    LOG_INF("WIFI INIT OK");

    if (this->set_wifi_credentials() != NET_ERROR::NET_OK)
    {
        return NET_ERROR::MISSING_WIFI_CREDENTIALS;
    }

    for (int attempt = 0; attempt <= MAX_ATTEMPT; attempt++)
    {
        ret = this->_wifi.connect_to_wifi();
        if (ret >= 0)
        {
            this->init_rssi_monitor();
            return NET_ERROR::NET_OK;
        }
        if (ret == -EIO)
        {
            return this->fail(NET_ERROR::IFACE_MISSING, "Network interface missing");
        }
        if (attempt < MAX_ATTEMPT)
        {
            LOG_WRN("Wifi connect retry - attemp [%d]", attempt);
            k_msleep(3000);
        }
    }
    return this->fail(NET_ERROR::WIFI_TIMEOUT, "Wifi timeout to connect");
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
void Netmgnt::Detach(IListener *listener)
{
}
void Netmgnt::Notify(Events evt)
{
    for (int i = 0; i < this->listener_count; i++)
    {
        this->listeners[i]->Update(evt);
        LOG_WRN("Update listener - %d",i);
    }
}
