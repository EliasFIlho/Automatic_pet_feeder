#pragma once

enum class Events
{
  WIFI_IFACE_UP,
  WIFI_IFACE_ERROR,
  WIFI_CREDS_OK,
  WIFI_CREDS_FAIL,
  WIFI_CONNECTED,
  WIFI_DISCONNECTED,
  IP_ACQUIRED,
  TIMEOUT,
  RETRY,
  MQTT_CONNECTED,
  MQTT_DISCONNECTED,
  MQTT_NEW_DATA
};

enum class EventGroup
{
  WIFI,
  MQTT,
  APP
};

struct EventMsg
{
  Events evt;
  EventGroup type;
};

class IListener
{
public:
  virtual ~IListener() {};
  virtual void Update(Events evt) = 0;
};
