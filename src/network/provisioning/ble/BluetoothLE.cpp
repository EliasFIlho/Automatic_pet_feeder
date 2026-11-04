#include "BluetoothLE.hpp"
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/hci.h>

BluetoothLE *BluetoothLE::s_self = nullptr;

LOG_MODULE_REGISTER(BLE_LOGS);

static const bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1)};

bt_conn_cb cb = {};

#define PROV_UUID_16(short16) BT_UUID_128_ENCODE(0xdfcfe733 | (short16), 0x4c03, 0x4cb2, 0x98b9, 0xa184e8313ec4)

static const struct bt_uuid_128 svc_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0001));
static const struct bt_uuid_128 ssid_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0002));
static const struct bt_uuid_128 pass_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0003));
static const struct bt_uuid_128 stat_uuid = BT_UUID_INIT_128(PROV_UUID_16(0x0004));

BT_GATT_SERVICE_DEFINE(prov_svc,
                       BT_GATT_PRIMARY_SERVICE(&svc_uuid),
                       BT_GATT_CHARACTERISTIC(&ssid_uuid.uuid,
                                              BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_WRITE,
                                              NULL, BluetoothLE::write_ssid, NULL),
                       BT_GATT_CUD("WiFi - SSID", BT_GATT_PERM_READ),
                       BT_GATT_CHARACTERISTIC(&pass_uuid.uuid,
                                              BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_WRITE,
                                              NULL, BluetoothLE::write_password, NULL),
                       BT_GATT_CUD("WiFi - PASSWORD", BT_GATT_PERM_READ),
                       BT_GATT_CHARACTERISTIC(&stat_uuid.uuid,
                                              BT_GATT_CHRC_NOTIFY,
                                              BT_GATT_PERM_NONE,
                                              NULL, NULL, NULL),
                       BT_GATT_CCC(NULL, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)),
                       BT_GATT_CUD("WiFi - CONNECTION STATUS", BT_GATT_PERM_READ), );

BluetoothLE::BluetoothLE(/* args */)
{
}

BluetoothLE::~BluetoothLE()
{
}

ssize_t BluetoothLE::write_ssid(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
                                uint16_t len, uint16_t offset, uint8_t flags)
{
    LOG_INF("Func %s: Called", __func__);

    // TODO: Update for kconfig constant
    if (len > sizeof(s_self->creds.ssid))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }
    memset(s_self->creds.ssid, 0, sizeof(s_self->creds.ssid));
    memcpy(s_self->creds.ssid, buf, len);
    s_self->creds.ssid_len = len;
    s_self->data_check();

    return len;
}

ssize_t BluetoothLE::write_password(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
                                    uint16_t len, uint16_t offset, uint8_t flags)
{
    LOG_INF("Func %s: Called", __func__);

    // TODO: Update for kconfig constant
    if (len > sizeof(s_self->creds.psk))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }
    memset(s_self->creds.psk, 0, sizeof(s_self->creds.psk));
    memcpy(s_self->creds.psk, buf, len);
    s_self->creds.psk_len = len;
    s_self->data_check();

    return len;
}

void BluetoothLE::connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        LOG_ERR("connection to %s failed (0x%02x %s)", bt_conn_dst_str(conn), err, bt_hci_err_to_str(err));
        return;
    }
    LOG_INF("%s: %s", __func__, bt_conn_dst_str(conn));
}
void BluetoothLE::disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("%s: %s (reason 0x%02x %s)", __func__, bt_conn_dst_str(conn), reason,
            bt_hci_err_to_str(reason));
}

void BluetoothLE::data_check()
{
    if (this->creds.psk_len != 0 && this->creds.ssid_len != 0)
    {
        LOG_WRN("COMMITING CREDENTIALS TO PROVISIONING");
        this->prov_cb(this->creds);
        this->clear_creds_buffer();
    }else{
        LOG_WRN("MISSING CREDENTIAL ELEMENT");
    }
}

void BluetoothLE::clear_creds_buffer()
{
    LOG_WRN("CLEANING CREDENTIALS");
    memset(this->creds.psk, 0, sizeof(this->creds.psk));
    this->creds.psk_len = 0;
    memset(this->creds.ssid, 0, sizeof(this->creds.ssid));
    this->creds.ssid_len = 0;
}

int BluetoothLE::adv_start(void)
{
    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);

    if (err == -EALREADY)
    {
        return 0;
    }

    if (err)
    {
        LOG_ERR("adv start failed (%d)", err);
        return err;
    }

    LOG_INF("advertising as '%s'", CONFIG_BT_DEVICE_NAME);

    return 0;
}

int BluetoothLE::adv_stop(void)
{
    int err = bt_le_adv_stop();

    if (err && err != -EALREADY)
    {
        LOG_ERR("adv stop failed (%d)", err);
        return err;
    }

    return 0;
}

int BluetoothLE::init(provisioning_callback callback_func)
{
    s_self = this;
    int err = bt_enable(NULL);
    if (err != 0)
    {
        LOG_ERR("Bluetooth enable failed (%d)", err);
        return err;
    }
    else
    {
        err = settings_load();
        if (err != 0)
        {
            LOG_ERR("ERROR TO LOAD SETTINGS: %d", err);
        }
        this->prov_cb = callback_func;
        cb.connected = BluetoothLE::connected;
        cb.disconnected = BluetoothLE::disconnected;
        if (bt_conn_cb_register(&cb) != 0)
        {
            LOG_WRN("Callback already register");
        }
        this->isBluetoothEnabled = true;
        return err;
    }
}

int BluetoothLE::stop()
{
    if (!this->isBluetoothEnabled)
    {
        LOG_WRN("BluetoothLE already disabled");
    }
    int ret = bt_disable();
    if (ret != 0)
    {
        LOG_ERR("ERROR TO DISABLE BLUETOOTH: %d", ret);
    }
    return ret;
}

void BluetoothLE::notify_connection_state(uint8_t status)
{
    bt_gatt_attr *attr;
    attr = bt_gatt_find_by_uuid(prov_svc.attrs, prov_svc.attr_count, &stat_uuid.uuid);
    bt_gatt_notify(NULL, attr, &status, sizeof(status));
}