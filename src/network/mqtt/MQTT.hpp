#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#define NUM_OF_TOPICS 2

extern k_sem update_rules;

class MQTT
{
private:
    struct mqtt_client client_ctx;
    struct zsock_pollfd fds[1];
    int nfds;
    struct sockaddr_in broker;
    struct mqtt_utf8 user_utf8;
    struct mqtt_utf8 pass_utf8;

    bool is_mqtt_connected;

    uint8_t rx_buffer[CONFIG_MQTT_RX_BUFFER_SIZE];
    uint8_t tx_buffer[CONFIG_MQTT_TX_BUFFER_SIZE];

    struct k_thread MQTTTask;

private:
    static void mqtt_read_payload_task(void *p1, void *, void *);
    static void mqtt_publish_payload_task(void *p1, void *, void *);
    void set_fds();
    int poll_mqtt_socket(int timout);
    bool init();
    bool connect();
    bool subscribe();
    bool setup_broker();
    bool setup_client();
    int read_payload();
    void on_disconnect();
    static void mqtt_evt_handler(struct mqtt_client *client, const struct mqtt_evt *evt);
    bool is_connected();
    void reconnect();

public:
    MQTT();
    ~MQTT();
    void start_mqtt();
};
