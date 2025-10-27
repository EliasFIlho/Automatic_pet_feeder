#include "NetworkService.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(NETWORK_LOGS);



NetworkService::NetworkService(IMQTT &mqtt, IWifi &wifi, IStorage &fs, ILed &led) : _mqtt(mqtt), _wifi(wifi), _fs(fs), _led(led)
{
}

NetworkService::~NetworkService()
{
}

bool NetworkService::is_mqtt_updated_payload()
{
    return false;
}

void dummy()
{
    return;
}

NET_ERROR NetworkService::start()
{
    char ssid[16];
    char psk[16];
    int ret;
    LOG_INF("Start Network LOGS");
    ret = this->_fs.read_data(SSID_ID, ssid, sizeof(ssid));
    if (ret < 0)
    {
        LOG_ERR("MISSING_WIFI_CREDENTIALS");
        this->indicate_error(NET_ERROR::MISSING_WIFI_CREDENTIALS);
        return NET_ERROR::MISSING_WIFI_CREDENTIALS;
    }
    else
    {
        this->_wifi.set_wifi_ssid(ssid);
    }

    ret = this->_fs.read_data(PASSWORD_ID, psk, sizeof(psk));
    if (ret < 0)
    {
        LOG_ERR("MISSING_WIFI_CREDENTIALS");
        this->indicate_error(NET_ERROR::MISSING_WIFI_CREDENTIALS);
        return NET_ERROR::MISSING_WIFI_CREDENTIALS;
    }
    else
    {
        this->_wifi.set_wifi_psk(psk);
    }

    if (this->_wifi.wifi_init())
    {
        LOG_INF("WIFI INIT OK");
        ret = this->_wifi.connect_to_wifi();
        if (ret < 0)
        {
            if (ret == -EIO)
            {
                LOG_ERR("IFACE_MISSING");
                this->indicate_error(NET_ERROR::IFACE_MISSING);
                return NET_ERROR::IFACE_MISSING;
            }
            else if (ret == -1)
            {
                LOG_ERR("WIFI_TIMEOUT");
                this->indicate_error(NET_ERROR::WIFI_TIMEOUT);
                return NET_ERROR::WIFI_TIMEOUT;
            }
            else
            {
                LOG_ERR("WIFI_INIT_ERROR");
                this->indicate_error(NET_ERROR::WIFI_INIT_ERROR);
                return NET_ERROR::WIFI_INIT_ERROR;
            }
        }
        else
        {
            LOG_INF("START RSSI MONITOR");
            this->init_rssi_monitor();
            LOG_INF("START MQTT");
            this->_mqtt.start_mqtt();
            return NET_ERROR::NET_OK;
        }
    }
    else
    {
        LOG_ERR("WIFI_INIT_ERROR");
        this->indicate_error(NET_ERROR::WIFI_INIT_ERROR);
        return NET_ERROR::WIFI_INIT_ERROR;
    }
}

void NetworkService::stop()
{
    this->_mqtt.abort();
    this->_wifi.wifi_disconnect();
}

int32_t NetworkService::init_rssi_monitor()
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

void NetworkService::indicate_error(NET_ERROR err)
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

void NetworkService::rssi_monitor(struct k_work *work)
{
    k_work_delayable *dwork = k_work_delayable_from_work(work);
    auto *self = CONTAINER_OF(dwork, NetworkService, rssi_monitor_work);

    int32_t rssi = self->_wifi.get_rssi();
    self->_led.set_mapped_output(rssi, CONFIG_RSSI_LOWER_VALUE, CONFIG_RSSI_HIGHER_VALUE);
    k_work_reschedule(dwork, K_SECONDS(CONFIG_RSSI_WORK_PERIOD));
}
