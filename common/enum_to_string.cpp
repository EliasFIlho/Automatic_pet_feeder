
#include "enum_to_string.hpp"

const char *EVENT_TO_STRING(Events evt)
{
    switch (evt)
    {
    case Events::START:
        return "START";
    case Events::WIFI_IFACE_UP:
        return "WIFI_IFACE_UP";
    case Events::WIFI_IFACE_ERROR:
        return "WIFI_IFACE_ERROR";
    case Events::WIFI_CREDS_OK:
        return "WIFI_CREDS_OK";
    case Events::WIFI_CREDS_FAIL:
        return "WIFI_CREDS_FAIL";
    case Events::WIFI_CREDS_NOT_FOUND:
        return "WIFI_CREDS_NOT_FOUND";
    case Events::WIFI_CONNECTED:
        return "WIFI_CONNECTED";
    case Events::WIFI_DISCONNECTED:
        return "WIFI_DISCONNECTED";
    case Events::WIFI_AP_ENABLE:
        return "WIFI_AP_ENABLE";
    case Events::WIFI_AP_DISABLED:
        return "WIFI_AP_DISABLED";
    case Events::WIFI_IP_ACQUIRED:
        return "WIFI_IP_ACQUIRED";
    case Events::TIMEOUT:
        return "TIMEOUT";
    case Events::MQTT_CONNECTED:
        return "MQTT_CONNECTED";
    case Events::MQTT_DISCONNECTED:
        return "MQTT_DISCONNECTED";
    case Events::MQTT_NEW_DATA:
        return "MQTT_NEW_DATA";
    default:
        return "";
    }
}

const char *STATE_TO_STRING(WifiSmState state)
{
    switch (state)
    {
    case WifiSmState::INITIALIZING:
        return "INITIALIZING";
    case WifiSmState::LOADING_CREDENTIALS:
        return "LOADING_CREDENTIALS";
    case WifiSmState::CONNECTING:
        return "CONNECTING";
    case WifiSmState::WAIT_IP:
        return "WAIT_IP";
    case WifiSmState::CONNECTED:
        return "CONNECTED";
    case WifiSmState::IFACE_ERROR:
        return "IFACE_ERROR";
    case WifiSmState::ENABLING_AP:
        return "ENABLING_AP";
    case WifiSmState::ENABLING_HTTP_SERVER:
        return "ENABLING_HTTP_SERVER";
    default:
        return "";
    }
}
