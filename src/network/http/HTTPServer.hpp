#pragma once

#include "IHTTPServer.hpp"
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>

class HTTPServer : public IHTTPServer
{
private:
    static struct http_resource_detail_dynamic connect_resource_detail;
    static struct http_resource_detail_static home_page_resource_detail;

private:
    static int connect_handler(struct http_client_ctx *client, enum http_data_status status, const struct http_request_ctx *request_ctx, struct http_response_ctx *response_ctx, void *user_data);
    void config_home_page_resource_struct();
    void config_connect_resource_struct();

public:
    void start();
    void stop();
    static struct http_resource_detail_dynamic *get_connect_resource();
    static struct http_resource_detail_static *get_home_resource();
    HTTPServer();
    ~HTTPServer();
};
