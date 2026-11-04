#pragma once
#include <zephyr/bluetooth/conn.h>
#include "IBluetoothLE.hpp"
#include "types.hpp"

class BluetoothLE : public IBluetoothLE
{

private:
    static BluetoothLE *s_self;
    bool isBluetoothEnabled = false;
    ProvCredentials creds;

private:
    provisioning_callback prov_cb;
    static void connected(struct bt_conn *conn, uint8_t err);
    static void disconnected(struct bt_conn *conn, uint8_t reason);
    void data_check();
    void clear_creds_buffer();

public:
    int init(provisioning_callback callback_func);
    int stop();
    int adv_start();
    int adv_stop();
    void notify_connection_state(uint8_t status);
    BluetoothLE();
    ~BluetoothLE();
    static ssize_t write_ssid(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
                              uint16_t len, uint16_t offset, uint8_t flags);
    static ssize_t write_password(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
                                  uint16_t len, uint16_t offset, uint8_t flags);
    static ssize_t read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                               uint16_t len, uint16_t offset);
};
