#pragma once
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include "MQTT_utils.hpp"
#include "IMQTT.hpp"
#include "IStorage.hpp"
#include "IWatchDog.hpp"

/**
 * @brief Extern semaphore to indicate that new data was writed in filesystem from MQTT
 *
 */
// extern k_sem update_rules;

/**
 * @brief MQTT Class
 *
 */
class MQTT : public IMQTT
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

    struct k_thread MQTTReadSubTask;
    struct k_thread MQTTPublishTask;

    struct publish_payload pub_payload;
    IWatchDog &_guard;
    IStorage &_fs;

private:
    static void mqtt_task(void *p1, void *, void *);
    void mqtt_publish_payload();
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

    MQTT(IWatchDog &guard, IStorage &_fs);
    ~MQTT();
    void start_mqtt();
    void abort();
    bool is_payload_updated();
};
