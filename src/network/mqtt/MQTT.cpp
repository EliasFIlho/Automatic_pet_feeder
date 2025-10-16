#include "MQTT.hpp"
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/task_wdt/task_wdt.h>


// MQTT Thread Stack
#define MQTT_THREAD_OPTIONS (K_FP_REGS | K_ESSENTIAL)
K_THREAD_STACK_DEFINE(MQTT_STACK_AREA, CONFIG_MQTT_THREAD_STACK_SIZE);

/**
 * @brief Handle publish event in MQTT callback
 *
 * @param client
 * @param evt
 */
void MQTT::on_mqtt_publish(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
    auto *self = static_cast<MQTT *>(client->user_data);
    int rc;
    char payload[500];

    rc = mqtt_read_publish_payload(client, payload, 500);
    if (rc < 0)
    {
        printk("Failed to read received MQTT payload [%d]", rc);
        return;
    }
    /* Place null terminator at end of payload buffer */
    payload[rc] = '\0';
    printk("MQTT payload received!");
    printk("topic: '%s', payload: %s", evt->param.publish.message.topic.topic.utf8, payload);
    self->_fs.write_data(RULES_ID, payload);
}

/**
 * @brief Event handle for MQTT events
 *
 * @param client
 * @param evt
 */
void MQTT::mqtt_evt_handler(struct mqtt_client *client,
                            const struct mqtt_evt *evt)
{
    auto *self = static_cast<MQTT *>(client->user_data);
    switch (evt->type)
    {
    case MQTT_EVT_CONNACK:

        printk("CONACK EVENT RECEIVED...\n\r");
        if (evt->result != 0)
        {
            printk("MQTT Event Connect failed [%d]", evt->result);
            break;
        }
        self->is_mqtt_connected = true;

        break;
    case MQTT_EVT_DISCONNECT:

        printk("MQTT_EVT_DISCONNECT Event\n\r");
        self->on_disconnect();

        break;
    case MQTT_EVT_PUBLISH:

        printk("DATA PUBLISHED\n\r");
        self->on_mqtt_publish(client, evt);

        break;
    case MQTT_EVT_PUBACK:
        if (evt->result == MQTT_SUBACK_FAILURE)
        {
            printk("MQTT SUBACK error [%d]", evt->result);
            break;
        }

        printk("SUBACK packet ID: %d", evt->param.suback.message_id);
        break;
    case MQTT_EVT_PUBREC:
        printk("MQTT_EVT_PUBREC Event\n\r");
        break;
    case MQTT_EVT_PUBREL:
        printk("MQTT_EVT_PUBREL Event\n\r");

        break;
    case MQTT_EVT_PUBCOMP:
        printk("MQTT_EVT_PUBCOMP Event\n\r");

        break;
    case MQTT_EVT_SUBACK:
        printk("MQTT_EVT_SUBACK Event\n\r");

        break;
    case MQTT_EVT_UNSUBACK:
        printk("MQTT_EVT_UNSUBACK Event\n\r");

        break;
    case MQTT_EVT_PINGRESP:
        printk("MQTT_EVT_PINGRESP Event\n\r");

        break;
    default:
        printk("Something trigged\n\r");
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
    this->is_mqtt_connected = false;
    this->nfds = 0;
}

/**
 * @brief Setup broker address
 *
 * @return true
 * @return false
 */
bool MQTT::setup_broker()
{

    this->broker.sin_family = AF_INET;
    this->broker.sin_port = htons(CONFIG_MQTT_BROKER_PORT);
    int ret = zsock_inet_pton(AF_INET, CONFIG_MQTT_BROKER_ADDR, &this->broker.sin_addr);
    if (ret != 1)
    {
        printk("SOCKET ERROR: CAN NOT CONVERT SERVER ADDRESS: %d\n\r", ret);
        return false;
    }
    else
    {
        printk("SOCKET EVENT: MQTT ADDRESS CONVERTED\n\r");
        return true;
    }
}

/**
 * @brief Set the file descript for event pollin
 *
 */
void MQTT::set_fds()
{
    if (this->client_ctx.transport.type == MQTT_TRANSPORT_NON_SECURE)
    {
        this->fds[0].fd = this->client_ctx.transport.tcp.sock;
    }

    this->fds[0].events = ZSOCK_POLLIN;
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
        printk("ERROR POLL [%d]", ret);
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

    this->is_mqtt_connected = false;
    int ret;
    /* Block until MQTT CONNACK event callback occurs */
    while (!this->is_connected())
    {
        ret = mqtt_connect(&this->client_ctx);
        if (ret != 0)
        {
            printk("MQTT Connect failed [%d]\n\r", ret);
            k_msleep(500);
            continue;
        }

        /* Poll MQTT socket for response */
        ret = poll_mqtt_socket(5000);
        if (ret > 0)
        {
            mqtt_input(&this->client_ctx);
        }

        if (!this->is_connected())
        {
            mqtt_abort(&this->client_ctx);
        }
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
        printk("MQTT ERROR: ERROR TO SUBSCRIBE IN TOPIC: %s -- RET: %d\n\r", CONFIG_MQTT_RULES_TOPIC, ret);
        return false;
    }
    else
    {
        printk("MQTT: SUBSCRIBED IN TOPIC: %s\n\r", CONFIG_MQTT_RULES_TOPIC);
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
                printk("MQTT Input failed [%d]", ret);
                return ret;
            }
            else
            {
            }
            /* Socket error */
            if (fds[0].revents & (ZSOCK_POLLHUP | ZSOCK_POLLERR))
            {
                printk("MQTT socket closed / error");
                return -ENOTCONN;
            }
        }
    }
    else
    {
        ret = mqtt_live(&this->client_ctx);
        if (ret != 0)
        {
            printk("MQTT Live failed [%d]", ret);
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
    return this->is_mqtt_connected;
}

bool MQTT::setup_client()
{
    this->init();

    if (this->setup_broker())
    {
        printk("MQTT Broker ready\n\r");
    }
    else
    {
        printk("MQTT ERROR TO SET BROKER\n\r");
        return false;
    }

    if (this->connect())
    {
        this->subscribe();
    }
    else
    {
        printk("MQTT ERROR TO CONNECT\n\r");
        return false;
    }
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
        printk("NO DATA TO PUBLISH\n\r");
    }
}

/**
 * @brief Perform a reconnect operation, try to connect with server again and resubscribe in topic
 *
 */
void MQTT::reconnect()
{
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
    auto *self = static_cast<MQTT *>(p1);
    self->setup_client();
    int read_mqtt_task_wdt_id = self->_guard.create_and_get_wtd_timer_id(CONFIG_MQTT_WATCHDOG_TIMEOUT_THREAD);
    while (true)
    {
        if (self->is_connected())
        {
            // Poll for incoming data
            self->read_payload();
            // Check for incoming data to publish
            self->mqtt_publish_payload();

            // Feed task watchdog
            self->_guard.feed(read_mqtt_task_wdt_id);
        }
        else
        {
            self->reconnect();
            self->_guard.feed(read_mqtt_task_wdt_id);
            k_msleep(CONFIG_MQTT_THREAD_RECONNECT_PERIOD);
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
    if(this->is_connected()){
        mqtt_disconnect(&this->client_ctx,NULL);
        mqtt_abort(&this->client_ctx);
    }else{
        mqtt_abort(&this->client_ctx);
    }
    k_thread_abort(this->MQTT_Thread_id);

}

bool MQTT::is_payload_updated()
{
    return false;
}