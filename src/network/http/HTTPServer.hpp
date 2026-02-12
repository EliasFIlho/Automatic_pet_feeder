#pragma once

#include "IHTTPServer.hpp"
#include "IJson.hpp"
#include "IStorage.hpp"
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include "Enums.hpp"
#include "NetEvents.hpp"

#define CREDENTIALS_MAX_BUF_LEN 16

typedef struct credentials_payload
{
    uint8_t ssid[CREDENTIALS_MAX_BUF_LEN];
    uint8_t password[CREDENTIALS_MAX_BUF_LEN];
} Credentials_data_t;

class HTTPServer : public IHTTPServer
{
private:
    static struct http_resource_detail_dynamic connect_resource_detail;
    static struct http_resource_detail_static home_page_resource_detail;
    static struct http_resource_detail_static success_page_resource_detail;
    static struct http_resource_detail_static error_page_resource_detail;

    IJson &_json;
    IStorage &_fs;
    Credentials_data_t credentials;

private:
    static int connect_handler(struct http_client_ctx *client, enum http_data_status status, const struct http_request_ctx *request_ctx, struct http_response_ctx *response_ctx, void *user_data);
    void config_home_page_resource_struct();
    void config_success_page_resource_struct();
    void config_error_page_resource_struct();
    void config_connect_resource_struct();
    int32_t parse_credentials_data(uint8_t *buff);
    int32_t write_credentials_data();
    void notify_evt(Events evt);

public:
    void start();
    void stop();
    static struct http_resource_detail_dynamic *get_connect_resource();
    static struct http_resource_detail_static *get_home_resource();
    static struct http_resource_detail_static *get_success_resource();
    static struct http_resource_detail_static *get_error_resource();
    HTTPServer(IJson &json, IStorage &fs);
    ~HTTPServer();
};
