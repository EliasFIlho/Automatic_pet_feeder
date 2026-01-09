#include "HTTPServer.hpp"
#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>

#define POST_PAYLOAD_MAX_LEN 512

LOG_MODULE_REGISTER(HTTP_LOGS);

static uint16_t http_service_port = 80;
struct http_resource_detail_dynamic HTTPServer::connect_resource_detail;
struct http_resource_detail_static HTTPServer::home_page_resource_detail;


static uint8_t index_html_gz[] = {
    #include "index.html.gz.inc"
};

static uint8_t sucess_html_gz[] = {
    #include "sucess.html.gz.inc"
};

static uint8_t error_html_gz[] = {
    #include "error.html.gz.inc"
};

HTTP_SERVICE_DEFINE(http_service, NULL, &http_service_port, 1, 10, NULL, NULL, NULL);

HTTP_RESOURCE_DEFINE(home_page, http_service, "/", HTTPServer::get_home_resource());
HTTP_RESOURCE_DEFINE(connect_data, http_service, "/save", HTTPServer::get_connect_resource());

void HTTPServer::config_home_page_resource_struct()
{
    HTTPServer::home_page_resource_detail = {
        .common = {
            .bitmask_of_supported_http_methods = BIT(HTTP_GET),
            .type = HTTP_RESOURCE_TYPE_STATIC,
            .content_encoding = "gzip",
            .content_type = "text/html"},
        .static_data = index_html_gz,
        .static_data_len = sizeof(index_html_gz)};
}
void HTTPServer::config_connect_resource_struct()
{
    HTTPServer::connect_resource_detail.cb = HTTPServer::connect_handler;
    HTTPServer::connect_resource_detail.common.bitmask_of_supported_http_methods = BIT(HTTP_POST);
    HTTPServer::connect_resource_detail.common.type = HTTP_RESOURCE_TYPE_DYNAMIC;
    HTTPServer::connect_resource_detail.user_data = this;
}

int HTTPServer::connect_handler(struct http_client_ctx *client, enum http_data_status status, const struct http_request_ctx *request_ctx, struct http_response_ctx *response_ctx, void *user_data)
{
    HTTPServer *self = static_cast<HTTPServer *>(user_data);
    static uint8_t post_payload[POST_PAYLOAD_MAX_LEN];
    static size_t cursor;

    if (status == HTTP_SERVER_DATA_ABORTED)
    {
        cursor = 0;
        return 0;
    }

    if (request_ctx->data_len + cursor > POST_PAYLOAD_MAX_LEN)
    {
        cursor = 0;
        return -ENOMEM;
    }

    memcpy(post_payload + cursor, request_ctx->data, request_ctx->data_len);
    cursor += request_ctx->data_len;

    if (status == HTTP_SERVER_DATA_FINAL)
    {
        LOG_INF("%s", post_payload);
        cursor = 0;
    }
}

HTTPServer::HTTPServer()
{
}

HTTPServer::~HTTPServer()
{
}
void HTTPServer::start()
{
    LOG_WRN("Setup static resources");
    this->config_home_page_resource_struct();
    this->config_connect_resource_struct();
    LOG_WRN("Starting HTTP Server");
    http_server_start();
}
void HTTPServer::stop()
{
    http_server_stop();
}

struct http_resource_detail_dynamic *HTTPServer::get_connect_resource()
{
    return &HTTPServer::connect_resource_detail;
}
struct http_resource_detail_static *HTTPServer::get_home_resource()
{
    return &HTTPServer::home_page_resource_detail;
}