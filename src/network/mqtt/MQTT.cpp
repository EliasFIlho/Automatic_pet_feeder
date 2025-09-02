#include "MQTT.hpp"
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

void mqtt_evt_handler(struct mqtt_client *client,
                      const struct mqtt_evt *evt)
{
    switch (evt->type)
    {
        /* Handle events here. */
    }
}

MQTT::MQTT(/* args */)
{
}

MQTT::~MQTT()
{
}

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

bool MQTT::connect()
{
    int ret = mqtt_connect(&this->client_ctx);
    if (ret != 0)
    {
        printk("MQTT ERROR: Unable to connect -> %d\n\r", ret);
        return false;
    }
    else
    {
        printk("MQTT: Connected\n\r");
        return true;
    }
}

bool MQTT::init()
{
    mqtt_client_init(&this->client_ctx);
    this->client_ctx.broker = &this->broker;
    this->client_ctx.evt_cb = mqtt_evt_handler;
    this->client_ctx.client_id.utf8 = (uint8_t *)"DEVICE_X";
    this->client_ctx.client_id.size = sizeof("DEVICE_X") - 1;

    this->user_utf8.utf8 = (uint8_t *)CONFIG_MQTT_DEVICE_LOGIN;
    this->user_utf8.size = sizeof(CONFIG_MQTT_DEVICE_LOGIN) - 1;
    this->client_ctx.user_name = &this->user_utf8;

    this->pass_utf8.utf8 = (uint8_t *)CONFIG_MQTT_DEVICE_PASSWORD;
    this->pass_utf8.size = sizeof(CONFIG_MQTT_DEVICE_PASSWORD) - 1;

    this->client_ctx.password = &this->pass_utf8;

    this->client_ctx.protocol_version = MQTT_VERSION_3_1_1;
    this->client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;

    this->client_ctx.rx_buf = this->rx_buffer;
    this->client_ctx.rx_buf_size = sizeof(this->rx_buffer);

    this->client_ctx.tx_buf = this->tx_buffer;
    this->client_ctx.tx_buf_size = sizeof(this->tx_buffer);

    return true;
}