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
    bool start();
    void stop();
    ~NetworkService();
};


