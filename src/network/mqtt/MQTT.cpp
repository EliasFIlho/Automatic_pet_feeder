#include "MQTT.hpp"
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>

#if CONFIG_MQTT_TLS_ENABLE
#include "tls_cert/cert.h"

#define MQTT_CA_CERT_TAG 1

static const sec_tag_t m_sec_tags[] = {
    MQTT_CA_CERT_TAG,
};

#endif

/**
 * @brief Thread stack and option defines
 *
 */
#define MQTT_THREAD_OPTIONS (K_FP_REGS | K_ESSENTIAL)
K_THREAD_STACK_DEFINE(MQTT_STACK_AREA, CONFIG_MQTT_THREAD_STACK_SIZE);

/**
 * @brief Construct a new log module register object
 *
 */
LOG_MODULE_REGISTER(MQTT_LOG);

/**
 * @brief JSON structure to parse + validate data
 *
 */
static const struct json_obj_descr rules_specific_date[] = {
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, year, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, month, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, day, JSON_TOK_UINT),
};

static const struct json_obj_descr rules_time[] = {
    JSON_OBJ_DESCR_PRIM(TimeRule_t, hour, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(TimeRule_t, minutes, JSON_TOK_UINT),
};

static const struct json_obj_descr rules_json_obj[] = {

    JSON_OBJ_DESCR_OBJECT(Rules_t, date, rules_specific_date),
    JSON_OBJ_DESCR_OBJECT(Rules_t, time, rules_time),
    JSON_OBJ_DESCR_PRIM(Rules_t, period, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, week_days, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, amount, JSON_TOK_UINT),

};

/**
 * @brief Parse payload
 * @param payload
 * @return true
 * @return false
 */
bool MQTT::parse_payload(char *payload, Rules_t *rules)
{

    int ret = this->_json.parse(payload, rules, rules_json_obj, ARRAY_SIZE(rules_json_obj));
    if (ret < 0)
    {
        LOG_ERR("ERROR TO PASE JSON FILE: %d", ret);
        return false;
    }
    else
    {
        LOG_INF("JSON FILE PARSED");

        return true;
    }
}

/**
 * @brief   Validate the receive payload data
 *
 *   - Check for valid data based on perid selected
        - Check if week day mask > 0 for weekly period
        - Check for month greater than 12 for specifc period
        - Check for days > 31
        - Not realy sure about year limitations :p
    - Check for corrupted data such as hour greater than 24 or minutes greater than 59

 *
 * @param rules
 * @return true
 * @return false
 */
bool MQTT::validate_payload(Rules_t *rules)
{
    if (rules->period == WEEKLY)
    {
        if (rules->week_days == 0 || rules->week_days > MAX_WEEK_DAYS_MASK_VALUE)
        {
            LOG_ERR("Wrong value for week days mask value equal to 0 or greater than 0x7F: %d", rules->week_days);
            return false;
        }
    }
    else if (rules->period == SPECIF)
    {
        if (rules->date.day > MAX_DAYS_SPECIFIC_VALUE || rules->date.month > MAX_MONTHS_SPECIFIC_VALUE ||
            rules->date.day < MIN_DAYS_SPECIFIC_VALUE || rules->date.month < MIN_MONTHS_SPECIFIC_VALUE)
        {
            LOG_ERR("Wrong date field in JSON: [ Day - %d | Month - %d ]", rules->date.day, rules->date.month);
            return false;
        }
    }
    else
    {
        LOG_ERR("Wrong value for period field in JSON: %d", rules->period);
        return false;
    }

    if (rules->time.hour > MAX_HOUR_TIME_VALUE || rules->time.minutes > MAX_MINUTE_TIME_VALUE)
    {
        LOG_ERR("Wrong time field in JSON: [ Hour - %d | Minutes - %d ]", rules->time.hour, rules->time.minutes);
        return false;
    }

    return true;
}

/**
 * @brief Handle publish event in MQTT callback
 *
 * @param client
 * @param evt
 */
void MQTT::on_mqtt_publish(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
    auto *self = static_cast<MQTT *>(client->user_data);
    int ret;
    char payload[500];
    ret = mqtt_read_publish_payload(client, payload, 500);
    if (ret < 0)
    {
        LOG_ERR("Failed to read received MQTT payload [%d]", ret);
        return;
    }
    /* Place null terminator at end of payload buffer */
    payload[ret] = '\0';
    Rules_t rules_payload;

    if (self->parse_payload(payload, &rules_payload))
    {
        if (self->validate_payload(&rules_payload))
        {
            ret = self->_fs.write_buffer(RULES_ID, &rules_payload, sizeof(rules_payload));
            if (ret >= 0)
            {
                self->notify_evt(Events::MQTT_NEW_DATA);
            }
        }
    }
}

/**
 * @brief Event handle for MQTT events
 *
 * @param client
 * @param evt
 */
void MQTT::mqtt_evt_handler(struct mqtt_client *client, const struct mqtt_evt *evt)
{
    auto *self = static_cast<MQTT *>(client->user_data);
    switch (evt->type)
    {
    case MQTT_EVT_CONNACK:

        if (evt->result != 0)
        {
            LOG_ERR("MQTT Event Connect failed [%d]", evt->result);
            break;
        }
        self->notify_evt(Events::MQTT_CONNECTED);
        self->isMqttConnected = true;

        break;
    case MQTT_EVT_DISCONNECT:

        LOG_WRN("MQTT_EVT_DISCONNECT Event");
        self->notify_evt(Events::MQTT_DISCONNECTED);
        self->on_disconnect();

        break;
    case MQTT_EVT_PUBLISH:

        self->on_mqtt_publish(client, evt);

        break;
    case MQTT_EVT_PUBACK:
        if (evt->result == MQTT_SUBACK_FAILURE)
        {
            LOG_ERR("MQTT SUBACK error [%d]", evt->result);
            break;
        }

        LOG_INF("SUBACK packet ID: %d", evt->param.suback.message_id);
        break;
    case MQTT_EVT_PUBREC:
        LOG_INF("MQTT_EVT_PUBREC Event");
        break;
    case MQTT_EVT_PUBREL:
        LOG_INF("MQTT_EVT_PUBREL Event");

        break;
    case MQTT_EVT_PUBCOMP:
        LOG_INF("MQTT_EVT_PUBCOMP Event");

        break;
    case MQTT_EVT_SUBACK:
        LOG_INF("MQTT_EVT_SUBACK Event");

        break;
    case MQTT_EVT_UNSUBACK:
        LOG_INF("MQTT_EVT_UNSUBACK Event");

        break;
    case MQTT_EVT_PINGRESP:
        LOG_INF("MQTT_EVT_PINGRESP Event");

        break;
    default:
        LOG_INF("Something trigged - [%d]", evt->type);
        break;
    }
}

/**
 * @brief Construct a new MQTT::MQTT object
 *
 */
MQTT::MQTT(IWatchDog &guard, IStorage &fs, IJson &json) : _guard(guard), _fs(fs), _json(json)
{
}

/**
 * @brief Destroy the MQTT::MQTT object
 *
 */
MQTT::~MQTT()
{
}

/**
 * @brief Handle disconnect event in MQTT
 *
 */
void MQTT::on_disconnect()
{
    this->isMqttConnected = false;
    this->nfds = 0;
}

/**
 * @brief Setup broker address
 *
 * @return true
 * @return false
 */

// TODO: Try to get the broker ip using getaddrinfo
bool MQTT::setup_broker()
{
    this->isBrokerSeted = false;
    int ret;
    this->broker.sin_family = AF_INET;
#if CONFIG_MQTT_TLS_ENABLE
    this->broker.sin_port = htons(CONFIG_MQTT_BROKER_PORT_TLS);
#else
    this->broker.sin_port = htons(CONFIG_MQTT_BROKER_PORT);
#endif
    ret = zsock_inet_pton(AF_INET, CONFIG_MQTT_BROKER_ADDR, &this->broker.sin_addr);
    if (ret != 1)
    {
        LOG_ERR("SOCKET ERROR: CAN NOT CONVERT SERVER ADDRESS: %d", ret);
        return false;
    }
    else
    {
        LOG_INF("SOCKET: MQTT ADDRESS CONVERTED");
        this->isBrokerSeted = true;
        return true;
    }
}

/**
 * @brief Set the file descript for event pollin
 *
 */
void MQTT::set_fds()
{
#if CONFIG_MQTT_TLS_ENABLE
    if (this->client_ctx.transport.type == MQTT_TRANSPORT_SECURE)
    {
        this->fds[0].fd = this->client_ctx.transport.tls.sock;
    }
#else

    if (this->client_ctx.transport.type == MQTT_TRANSPORT_NON_SECURE)
    {
        this->fds[0].fd = this->client_ctx.transport.tcp.sock;
    }

#endif

    this->fds[0].events = ZSOCK_POLLIN;
    this->fds->revents = 0;
    this->nfds = 1;
}

/**
 * @brief Perform poll operation for mqtt input data, return zsock_poll ret
 *
 * @param timout
 * @return int
 */
int MQTT::poll_mqtt_socket(int timout)
{
    this->set_fds();
    int ret = zsock_poll(this->fds, this->nfds, timout);
    if (ret < 0)
    {
        LOG_ERR("ERROR POLL [%d]", ret);
    }

    return ret;
}

/**
 * @brief Perform MQTT connect
 *
 * @return true
 * @return false
 */
bool MQTT::connect()
{

    this->isMqttConnected = false;
    int ret;

    ret = mqtt_connect(&this->client_ctx);
    if (ret != 0)
    {
        LOG_ERR("MQTT Connect failed [%d]", ret);
        return false;
    }

    /* Poll MQTT socket for response */
    ret = poll_mqtt_socket(5000);
    if (ret > 0)
    {
        mqtt_input(&this->client_ctx);
    }

    return true;
}

/**
 * @brief Subscribe in a specific topic (This is not a generic subscribe method, but specific for the application)
 *
 * @return true
 * @return false
 */
bool MQTT::subscribe()
{
    int ret;
    struct mqtt_topic sub_topics[] = {
        {.topic = {
             .utf8 = (uint8_t *)CONFIG_MQTT_RULES_TOPIC,
             .size = sizeof(CONFIG_MQTT_RULES_TOPIC) - 1},
         .qos = 0}};

    const struct mqtt_subscription_list sub_list = {
        .list = sub_topics,
        .list_count = ARRAY_SIZE(sub_topics),
        .message_id = 5841u};

    ret = mqtt_subscribe(&this->client_ctx, &sub_list);
    if (ret < 0)
    {
        LOG_ERR("MQTT ERROR: ERROR TO SUBSCRIBE IN TOPIC: %s -- RET: %d", CONFIG_MQTT_RULES_TOPIC, ret);
        return false;
    }
    else
    {
        LOG_INF("MQTT: SUBSCRIBED IN TOPIC: %s", CONFIG_MQTT_RULES_TOPIC);
        return true;
    }
}

/**
 * @brief Init MQTT client struct
 *
 * @return true
 * @return false
 */
void MQTT::init()
{

    mqtt_client_init(&this->client_ctx);
    this->client_ctx.broker = &this->broker;
    this->client_ctx.evt_cb = this->mqtt_evt_handler;
    this->client_ctx.user_data = this;
    // Config Device Name
    this->client_ctx.client_id.utf8 = (uint8_t *)CONFIG_DEVICE_ID;
    this->client_ctx.client_id.size = sizeof(CONFIG_DEVICE_ID) - 1;

    // Config Device Login
    this->user_utf8.utf8 = (uint8_t *)CONFIG_MQTT_DEVICE_LOGIN;
    this->user_utf8.size = sizeof(CONFIG_MQTT_DEVICE_LOGIN) - 1;
    this->client_ctx.user_name = &this->user_utf8;

    // Config Device Password
    this->pass_utf8.utf8 = (uint8_t *)CONFIG_MQTT_DEVICE_PASSWORD;
    this->pass_utf8.size = sizeof(CONFIG_MQTT_DEVICE_PASSWORD) - 1;
    this->client_ctx.password = &this->pass_utf8;

    // Config MQTT Version and Transport Layer
    this->client_ctx.protocol_version = MQTT_VERSION_3_1_1;
    this->client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;

    // Config MQTT RX and TX Buffers
    this->client_ctx.rx_buf = this->rx_buffer;
    this->client_ctx.rx_buf_size = sizeof(this->rx_buffer);

    this->client_ctx.tx_buf = this->tx_buffer;
    this->client_ctx.tx_buf_size = sizeof(this->tx_buffer);
}

/**
 * @brief Read incoming payload data, and perform keep alive request for broker after pollin timout
 *
 * @return int
 */
int MQTT::read_payload()
{
    int ret = this->poll_mqtt_socket(mqtt_keepalive_time_left(&this->client_ctx));
    if (ret != 0)
    {
        if (this->fds[0].revents & ZSOCK_POLLIN)
        {
            ret = mqtt_input(&this->client_ctx);
            if (ret != 0)
            {
                LOG_ERR("MQTT Input failed [%d]", ret);
                return ret;
            }
            /* Socket error */
            if (fds[0].revents & (ZSOCK_POLLHUP | ZSOCK_POLLERR))
            {
                LOG_ERR("MQTT socket closed / error");
                return -ENOTCONN;
            }
        }
    }
    else
    {
        ret = mqtt_live(&this->client_ctx);
        if (ret != 0)
        {
            LOG_ERR("MQTT Live failed [%d]", ret);
            return ret;
        }
    }
    return 0;
}

/**
 * @brief Return connect status of MQTT connection
 *
 * @return true
 * @return false
 */
bool MQTT::is_connected()
{
    return this->isMqttConnected;
}

bool MQTT::setup_client()
{
    this->init();
#if CONFIG_MQTT_TLS_ENABLE

    if (this->setup_tls() != 0)
    {
        return false;
    }
#endif

    return true;
}

void MQTT::populate_payload_struct(struct mqtt_binstr *payload, struct level_sensor *data)
{
    this->_json.encode(data, this->publish_buf, sizeof(this->publish_buf));
    payload->data = (uint8_t *)this->publish_buf;
    payload->len = strlen(this->publish_buf);
}

/**
 * @brief Gets data from a queue and publish in the specific topic
 *
 *
 */
void MQTT::mqtt_publish_payload()
{
    struct level_sensor data;
    static uint16_t msg_id = 1;

    if (k_msgq_get(&mqtt_publish_queue, &data, K_NO_WAIT) == 0)
    {
        struct mqtt_binstr payload;
        this->populate_payload_struct(&payload, &data);

        struct mqtt_topic topic = {
            .topic = {
                .utf8 = (uint8_t *)CONFIG_MQTT_SENSOR_TOPIC,
                .size = strlen(CONFIG_MQTT_SENSOR_TOPIC)},
            .qos = 0};

        struct mqtt_publish_param publish;
        publish.message.payload = payload;
        publish.message.topic = topic;
        publish.message_id = msg_id++;
        publish.dup_flag = 0;
        publish.retain_flag = 0;

        mqtt_publish(&this->client_ctx, &publish);
    }
    else
    {
    }
}

/**
 * @brief Perform a reconnect operation, try to connect with server again and resubscribe in topic
 *
 */
void MQTT::reconnect()
{
    if (!this->isBrokerSeted)
    {
        if (!this->setup_broker())
        {
            LOG_WRN("Could not setup broker...");
            return;
        }
    }

    if (this->connect())
    {
        this->subscribe();
    }
}

/**
 * @brief Task for handle broker publish in subscribed topic. This task keeps performing a read_payload call thats poll over incoming data
 * if the connection with broker was lost the task also keeps trying to reconnect every 5 seconds
 *
 * @param p1 p1 will be a pointer to class "this" pointer and enable acess to private data from class in static method
 */
void MQTT::mqtt_task(void *p1, void *, void *)
{
    // reference to "this" pointer
    auto *self = static_cast<MQTT *>(p1);
    int read_mqtt_task_wdt_id = self->_guard.create_and_get_wtd_timer_id(CONFIG_MQTT_WATCHDOG_TIMEOUT_THREAD);
    self->state = MQTT_STATES::INIT;
    while (true)
    {
        switch (self->state)
        {
        case MQTT_STATES::INIT:
            if (self->setup_client())
            {
                self->state = MQTT_STATES::CLIENT_READY;
            }
            else
            {
                //LOG_WRN("Client init failed - retrying");
                self->_guard.feed(read_mqtt_task_wdt_id);
                k_msleep(1000);
            }
            break;

        case MQTT_STATES::CLIENT_READY:
            if (self->setup_broker())
            {
                self->state = MQTT_STATES::BROKER_READY;
            }
            else
            {
                //LOG_WRN("Broker init failed - retrying");
                self->_guard.feed(read_mqtt_task_wdt_id);
                k_msleep(1000);
            }
            break;

        case MQTT_STATES::BROKER_READY:
            if (self->connect())
            {
                self->state = MQTT_STATES::CONNECTING;
            }
            else
            {
                //LOG_WRN("Fail to connect - retrying");
                self->_guard.feed(read_mqtt_task_wdt_id);
                k_msleep(CONFIG_MQTT_THREAD_RECONNECT_PERIOD);
            }
            break;

        case MQTT_STATES::CONNECTING:
            if (self->isMqttConnected)
            {
                if (self->subscribe())
                {
                    self->state = MQTT_STATES::RUNNING;
                }
                else
                {
                    self->state = MQTT_STATES::ERROR;
                }
            }
            else
            {
                //LOG_WRN("Waiting for CONACK");
                self->_guard.feed(read_mqtt_task_wdt_id);
                k_msleep(CONFIG_MQTT_THREAD_RECONNECT_PERIOD);
            }
            break;
        case MQTT_STATES::RUNNING:
            if (self->isMqttConnected)
            {
                int ret = self->read_payload();
                if (ret == -ENOTCONN)
                {
                    self->state = MQTT_STATES::ERROR;
                }
                else
                {
                    self->mqtt_publish_payload();
                    self->_guard.feed(read_mqtt_task_wdt_id);
                }
            }
            else
            {
                self->state = MQTT_STATES::ERROR;
            }
            break;

        case MQTT_STATES::ERROR:
            if (self->isWifiConnected)
            {
                self->state = MQTT_STATES::CLIENT_READY;
            }
            else
            {
                self->state = MQTT_STATES::ERROR; // Block state machine in ERROR state until network manager set wifi on back
                self->_guard.feed(read_mqtt_task_wdt_id);
                k_msleep(CONFIG_MQTT_THREAD_PERIOD);
            }
            break;

        default:
            break;
        }
    }
}

/**
 * @brief Create MQTT tasks
 *
 */
void MQTT::start_mqtt()
{
    this->MQTT_Thread_id = k_thread_create(&this->MQTTReadSubTask, MQTT_STACK_AREA, CONFIG_MQTT_THREAD_STACK_SIZE, MQTT::mqtt_task, this, NULL, NULL, CONFIG_MQTT_THREAD_PRIORITY, MQTT_THREAD_OPTIONS, K_NO_WAIT);
}

void MQTT::abort()
{
    if (this->is_connected())
    {
        mqtt_disconnect(&this->client_ctx, NULL);
        mqtt_abort(&this->client_ctx);
    }
    else
    {
        mqtt_abort(&this->client_ctx);
    }
    k_thread_abort(this->MQTT_Thread_id);
}

#if CONFIG_MQTT_TLS_ENABLE
int32_t MQTT::setup_tls()
{

    struct mqtt_sec_config *tls_config;
    this->client_ctx.transport.type = MQTT_TRANSPORT_SECURE;
    int ret;

    ret = tls_credential_add(MQTT_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, ca_certificate, sizeof(ca_certificate));
    if (ret < 0)
    {
        LOG_ERR("Failed to register public certificate: %d", ret);
        return ret;
    }
    tls_config = &this->client_ctx.transport.tls.config;
    tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
    tls_config->cipher_list = NULL;
    tls_config->sec_tag_list = m_sec_tags;
    tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
    tls_config->hostname = "broker.local";
    return 0;
}
#endif

void MQTT::notify_evt(Events evt)
{
    EventMsg msg{.evt = evt,
                 .type = EventGroup::MQTT};
    k_msgq_put(&net_evt_queue, &msg, K_NO_WAIT);
}

void MQTT::block_mqtt()
{
    this->isWifiConnected = false;
}
void MQTT::release_mqtt()
{
    this->isWifiConnected = true;
}