#pragma once
#include "NetEvents.hpp"
#include "Enums.hpp"
#include "types.hpp"
#include "IProvisioning.hpp"
#include "IBluetoothLE.hpp"
#include <zephyr/net/wifi_credentials.h>

/*
This is suppose to be a generic interface for provisioning
It will be used by the netmgnt module when it needs to get data

This module needs to be able to:
    1 - enable a provisioning method using the BLE module
    2 - control the provisioning data flow
    3 - Notify the netmgnt when credentials arrives
    4 - Notify thought the provisioning portal if the connection process was finished (this can be done with a public call from the netmgnt)



*/

class Provisioning : public IProvisioning
{

private:
    bool isProvisioningEnabled = false;
    IBluetoothLE &_ble;
    static Provisioning *s_self;
    char data_buf[256];
    ProvCredentials creds;

private:
    void notify_evt(Events evt, void *data);
    static void incoming_data_callback(ProvCredentials &creds);

public:
    int start_provisioning();
    int stop_provisioning();
    bool isProvisioningEnabled_();
    void notify_connection_status(uint8_t status);
    void write_credentials();
    Provisioning(IBluetoothLE &ble);
    ~Provisioning();
};