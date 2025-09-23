#pragma once
#include "IMQTT.hpp"
#include "IWifi.hpp"
#include "IStorage.hpp"
struct net_cfg
{
    
};


class NetworkService
{
private:
    IMQTT& _mqtt;
    IWifi& _wifi;
    IStorage& _fs;


public:
    NetworkService(IMQTT &mqtt, IWifi &wifi, IStorage& fs);
    bool is_mqtt_updated_payload();
    bool start();
    void stop();
    ~NetworkService();
};


