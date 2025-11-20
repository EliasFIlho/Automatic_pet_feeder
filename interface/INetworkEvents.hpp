#pragma once

enum class NetworkEvent {
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    GOT_IP,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    MQTT_NEW_DATA
};

struct NetEventMsg
{
    NetworkEvent evt;
};


class INetworkEvents {
public:
    virtual ~INetworkEvents() = default;
    virtual void on_network_event(NetworkEvent evt) = 0;
};