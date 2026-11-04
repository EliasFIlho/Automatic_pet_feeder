#include "Provisioning.hpp"
#include "BluetoothLE.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PROVISIONING_LOGS);

Provisioning *Provisioning::s_self = nullptr;

int Provisioning::start_provisioning()
{
    s_self = this;
    int ret = this->_ble.init(this->incoming_data_callback);
    if (ret != 0)
    {
        LOG_ERR("ERROR TO INIT BLE: %d", ret);
        return ret;
    }

    ret = _ble.adv_start();
    if (ret != 0)
    {
        LOG_ERR("ERROR TO START BLE ADVERTISE: %d", ret);
        return ret;
    }
    this->isProvisioningEnabled = true;
    return 0;
}

void Provisioning::incoming_data_callback(ProvCredentials &creds)
{
    s_self->creds = creds;
    LOG_WRN("| SSID: %s / PSK: %s |", s_self->creds.ssid, s_self->creds.psk);
    LOG_WRN("CREDENTIALS RECEIVED, NOTIFYING NET MANAGER");
    s_self->notify_evt(Events::PROVISIONING_CREDS_RECEIVED, NULL);
}

int Provisioning::stop_provisioning()
{
    int ret = this->_ble.adv_stop();
    if (ret != 0)
    {
        LOG_ERR("ERROR TO STOP BLE ADVERTISE: %d", ret);
    }
    this->isProvisioningEnabled = false;
    return ret;
}
bool Provisioning::isProvisioningEnabled_()
{
    return this->isProvisioningEnabled;
}
void Provisioning::notify_connection_status(uint8_t status)
{
    this->_ble.notify_connection_state(status);
}

void Provisioning::notify_evt(Events evt, void *data)
{
    EventMsg msg{.evt = evt,
                 .type = PROV_EVT,
                 .data = data};
    k_msgq_put(&net_evt_queue, &msg, K_NO_WAIT);
}

Provisioning::Provisioning(IBluetoothLE &ble) : _ble(ble)
{
}

Provisioning::~Provisioning()
{
}

void Provisioning::write_credentials()
{
    int ret = wifi_credentials_set_personal(this->creds.ssid, this->creds.ssid_len, WIFI_SECURITY_TYPE_PSK, NULL, 0,
                                            this->creds.psk, this->creds.psk_len, 0, 0, 0);

    if (ret != 0)
    {
        LOG_ERR("ERROR TO WRITE CREDENTIALS");
        this->notify_evt(Events::PROVISIONING_STORE_CREDENTIALS_ERROR, NULL);
    }
    else
    {
        this->notify_evt(Events::PROVISIONING_STORE_CREDENTIALS_OK, NULL);
        LOG_INF("CREDENTIALS WRITTEN");
    }
}