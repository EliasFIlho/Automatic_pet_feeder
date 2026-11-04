
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
    case Events::WIFI_IP_ACQUIRED:
        return "WIFI_IP_ACQUIRED";
    case Events::TIMEOUT:
        return "TIMEOUT";
    case Events::MQTT_CONNECTED:
        return "MQTT_CONNECTED";
    case Events::MQTT_DISCONNECTED:
        return "MQTT_DISCONNECTED";
    case Events::MQTT_UPDATE_DATA:
        return "MQTT_UPDATE_DATA";
    case Events::PROVISIONING_CREDS_RECEIVED:
        return "PROVISIONING_CREDS_RECEIVED";
    case Events::SHUTDOWN_FINISHED:
        return "SHUTDOWN_FINISHED";
    default:
        return "UNKNOWN";
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
    case WifiSmState::WAITING_FOR_IP_ADDRESS:
        return "WAITING_FOR_IP_ADDRESS";
    case WifiSmState::CONNECTED:
        return "CONNECTED";
    case WifiSmState::IFACE_ERROR:
        return "IFACE_ERROR";
    case WifiSmState::PROVISIONING:
        return "PROVISIONING";
    case WifiSmState::STORING_CRDENTIALS:
        return "STORING_CRDENTIALS";
    case WifiSmState::RESTART:
        return "RESTART";
    default:
        return "UNKNOWN";
    }
}

const char *STATE_TO_STRING(MQTT_STATES state)
{
    switch (state)
    {
    case MQTT_STATES::INIT:
        return "INITIALIZING";
    case MQTT_STATES::CLIENT_READY:
        return "CLIENT_READY";
    case MQTT_STATES::BROKER_READY:
        return "BROKER_READY";
    case MQTT_STATES::CONNECTING:
        return "CONNECTING";
    case MQTT_STATES::RUNNING:
        return "RUNNING";
    case MQTT_STATES::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}
