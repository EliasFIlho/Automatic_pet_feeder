#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

class MQTT
{
private:
    struct mqtt_client client_ctx;
    struct sockaddr_in broker;
    struct mqtt_utf8 user_utf8;
    struct mqtt_utf8 pass_utf8;

    uint8_t rx_buffer[1024];
    uint8_t tx_buffer[1024];

public:
    MQTT();
    ~MQTT();
    bool init();
    bool connect();
    bool setup_broker();
};
