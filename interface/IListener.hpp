#pragma once


enum class Events{
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    MQTT_NEW_DATA
};

class IListener
{
public:
  virtual ~IListener() {};
  virtual void Update(Events evt) = 0;
};

