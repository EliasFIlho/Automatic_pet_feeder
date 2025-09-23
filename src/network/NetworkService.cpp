#include "NetworkService.hpp"
NetworkService::NetworkService(IMQTT &mqtt, IWifi &wifi, IStorage &fs) : _mqtt(mqtt), _wifi(wifi), _fs(fs)
{
}

NetworkService::~NetworkService()
{
}

bool NetworkService::is_mqtt_updated_payload(){
    
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
