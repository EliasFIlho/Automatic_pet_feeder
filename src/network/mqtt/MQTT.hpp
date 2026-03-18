#pragma once
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include "MQTT_utils.hpp"
#include "IMQTT.hpp"
#include "IWatchDog.hpp"
#include "IJson.hpp"
#include "NetEvents.hpp"
#include "Enums.hpp"
#include "types.hpp"
#include "ISchedulerRules.hpp"



extern k_msgq mqtt_publish_queue;


#define MAX_WEEK_DAYS_MASK_VALUE 0x7F

#define MAX_DAYS_SPECIFIC_VALUE 31
#define MAX_MONTHS_SPECIFIC_VALUE 12
#define MIN_DAYS_SPECIFIC_VALUE 1
#define MIN_MONTHS_SPECIFIC_VALUE 1

#define MAX_HOUR_TIME_VALUE 23
#define MAX_MINUTE_TIME_VALUE 59


/**
 * @brief MQTT Class
 *
 */
class MQTT : public IMQTT
{
private:
    struct mqtt_client client_ctx;
    struct zsock_pollfd fds[1];
    MQTT_STATES state;
    int nfds;
    struct sockaddr_in broker;
    struct mqtt_utf8 user_utf8;
    struct mqtt_utf8 pass_utf8;
    bool isMqttConnected = false;
    bool isBrokerSeted = false;
    bool isWifiConnected = false;
    char publish_buf[CONFIG_MQTT_PUBLISH_BUFFER_SIZE];
    uint8_t rx_buffer[CONFIG_MQTT_RX_BUFFER_SIZE];
    uint8_t tx_buffer[CONFIG_MQTT_TX_BUFFER_SIZE];
    struct k_thread MQTTReadSubTask;
    k_tid_t MQTT_Thread_id;
    IWatchDog &_guard;
    IJson &_json;
    ISchedulerRules &_rules;


private:
    static void mqtt_task(void *p1, void *, void *);
    void mqtt_publish_payload();
    void set_fds();
    int poll_mqtt_socket(int timout);
    void init();
    bool connect();
    bool subscribe();
    bool setup_broker();
    bool setup_client();
    int read_payload();
    void on_disconnect();
    static void mqtt_evt_handler(struct mqtt_client *client, const struct mqtt_evt *evt);
    bool is_connected();
    void reconnect();
    void populate_payload_struct(struct mqtt_binstr *payload, struct level_sensor *data);
    static void on_mqtt_publish(struct mqtt_client *const client, const struct mqtt_evt *evt);
#if CONFIG_MQTT_TLS_ENABLE
    int32_t setup_tls();
#endif
    bool parse_payload(char *payload, Rules_t *rules);
    bool validate_payload(Rules_t *rules);
    void notify_evt(Events evt);

public:
    MQTT(IWatchDog &guard, IJson &json, ISchedulerRules &rules);
    ~MQTT();
    void start_mqtt();
    void block_mqtt();
    void release_mqtt();
    void abort();
};
