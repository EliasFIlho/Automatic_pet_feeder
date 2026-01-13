#pragma once

enum class Events
{
    START,
    WIFI_IFACE_UP,
    WIFI_IFACE_ERROR,
    WIFI_CREDS_OK,
    WIFI_CREDS_FAIL,
    WIFI_CREDS_NOT_FOUND,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    WIFI_AP_ENABLE,
    WIFI_AP_DISABLED,
    IP_ACQUIRED,
    TIMEOUT,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    MQTT_NEW_DATA
};

enum class WifiSmState
{
    INITIALIZING,
    LOADING_CREDENTIALS,
    CONNECTING,
    WAIT_IP,
    CONNECTED,
    IFACE_ERROR,
    ENABLING_AP,
    ENABLING_HTTP_SERVER
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

enum
{
    LOW,
    HIGH,
    FADE
};

enum class COLOR
{
    RED,
    GREEN,
    BLUE,
    YELLOW
};