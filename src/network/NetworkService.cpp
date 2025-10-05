#include "NetworkService.hpp"

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

bool NetworkService::start()
{
    char ssid[16];
    char psk[16];
    int ret;

    ret = this->_fs.read_data(SSID_ID, ssid, sizeof(ssid));
    if (ret < 0)
    {
        return false;
    }
    else
    {
        this->_wifi.set_wifi_ssid(ssid);
    }

    ret = this->_fs.read_data(PASSWORD_ID, psk, sizeof(psk));
    if (ret < 0)
    {
        return false;
    }
    else
    {
        this->_wifi.set_wifi_psk(psk);
    }

    if (this->_wifi.wifi_init())
    {
        if (this->_wifi.is_connected())
        {
            this->_wifi.wifi_disconnect();
        }
        ret = this->_wifi.connect_to_wifi();
        if (ret < 0)
        {
            this->stop();
            return false;
        }
        else
        {
            this->init_rssi_monitor();
            this->_mqtt.start_mqtt();
            return true;
        }
    }
    else
    {
        return false;
    }
}

void NetworkService::stop()
{
    this->_mqtt.abort();
    this->_wifi.wifi_disconnect();
}


/*TODO: Turn this into a thread to use the this pointer instead of CONTAINER_OF 
(No reason to do that, i just think that will be more easy to understand and also keep a standard pattern to periodic works)

*/

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

void NetworkService::rssi_monitor(struct k_work *work)
{
    k_work_delayable *dwork = k_work_delayable_from_work(work);
    auto *self = CONTAINER_OF(dwork, NetworkService, rssi_monitor_work);

    int32_t rssi = self->_wifi.get_rssi();
    self->_led.set_mapped_output(rssi,-90,-30);
    printk("RSSI FROM NETWORK SERVICE: [%d]\n\r", rssi);
    k_work_reschedule(dwork, K_MSEC(CONFIG_WIFI_RSSI_MONITOR_PERIOD));
}
