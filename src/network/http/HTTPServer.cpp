#include "HTTPServer.hpp"
#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>

#define POST_PAYLOAD_MAX_LEN 512

LOG_MODULE_REGISTER(HTTP_LOGS);

static const struct json_obj_descr json_credentials[] = {

    JSON_OBJ_DESCR_PRIM(Credentials_data_t, ssid, JSON_TOK_STRING_BUF),
    JSON_OBJ_DESCR_PRIM(Credentials_data_t, password, JSON_TOK_STRING_BUF),

};

static uint16_t http_service_port = 80;
struct http_resource_detail_dynamic HTTPServer::connect_resource_detail;
struct http_resource_detail_static HTTPServer::home_page_resource_detail;
struct http_resource_detail_static HTTPServer::error_page_resource_detail;
struct http_resource_detail_static HTTPServer::success_page_resource_detail;

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static uint8_t success_html_gz[] = {
#include "success.html.gz.inc"
};

static uint8_t error_html_gz[] = {
#include "error.html.gz.inc"
};

HTTP_SERVICE_DEFINE(http_service, NULL, &http_service_port, 1, 10, NULL, NULL, NULL);

HTTP_RESOURCE_DEFINE(home_page, http_service, "/", HTTPServer::get_home_resource());
HTTP_RESOURCE_DEFINE(error_page, http_service, "/error", HTTPServer::get_error_resource());
HTTP_RESOURCE_DEFINE(success_page, http_service, "/success", HTTPServer::get_success_resource());
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

void HTTPServer::config_success_page_resource_struct()
{
    HTTPServer::success_page_resource_detail = {
        .common = {
            .bitmask_of_supported_http_methods = BIT(HTTP_GET),
            .type = HTTP_RESOURCE_TYPE_STATIC,
            .content_encoding = "gzip",
            .content_type = "text/html"},
        .static_data = success_html_gz,
        .static_data_len = sizeof(success_html_gz)};
}

void HTTPServer::config_error_page_resource_struct()
{
    HTTPServer::error_page_resource_detail = {
        .common = {
            .bitmask_of_supported_http_methods = BIT(HTTP_GET),
            .type = HTTP_RESOURCE_TYPE_STATIC,
            .content_encoding = "gzip",
            .content_type = "text/html"},
        .static_data = error_html_gz,
        .static_data_len = sizeof(error_html_gz)};
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
        cursor = 0;
        LOG_INF("%s", post_payload);
        int32_t ret_parser = self->parse_credentials_data(post_payload);
        int32_t ret_fs_write = self->write_credentials_data();
        if ((ret_parser != 0) && (ret_fs_write < 0))
        {
            response_ctx->status = HTTP_400_BAD_REQUEST;
            LOG_ERR("BAD REQUEST");
        }
        else
        {
            response_ctx->status = HTTP_200_OK;
            LOG_WRN("REQUEST OK");
        }

        response_ctx->body = NULL;
        response_ctx->body_len = 0;
        response_ctx->final_chunk = true;

        // Clear buffers
        // memset(post_payload, 0, POST_PAYLOAD_MAX_LEN);
        // memset(&self->credentials, 0, sizeof(self->credentials));
        return 0;
    }
    return 0;
}

HTTPServer::HTTPServer(IJson &json, IStorage &fs) : _json(json), _fs(fs)
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
    this->config_success_page_resource_struct();
    this->config_error_page_resource_struct();
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

struct http_resource_detail_static *HTTPServer::get_success_resource()
{
    return &HTTPServer::success_page_resource_detail;
}

struct http_resource_detail_static *HTTPServer::get_error_resource()
{
    return &HTTPServer::error_page_resource_detail;
}

int32_t HTTPServer::parse_credentials_data(uint8_t *buff)
{
    const int expected_return_code = BIT_MASK(ARRAY_SIZE(json_credentials));
    int32_t ret = this->_json.parse((char *)buff, &this->credentials, json_credentials, ARRAY_SIZE(json_credentials));
    if (ret != expected_return_code)
    {
        LOG_ERR("Error to parse data");
        return -1;
    }
    else
    {
        LOG_WRN("DATA: SSID [%s] - PSK [%s]", this->credentials.ssid, this->credentials.password);
        return 0;
    }
}
// TODO: This part is breaking the system, why? we'll see
int32_t HTTPServer::write_credentials_data()
{
    size_t ssid_len = strnlen((char *)this->credentials.ssid, CREDENTIALS_MAX_BUF_LEN);
    size_t pass_len = strnlen((char *)this->credentials.password, CREDENTIALS_MAX_BUF_LEN);
    int32_t ret = this->_fs.write_buffer(SSID_ID, this->credentials.ssid, ssid_len);
    if (ret < 0)
    {
        LOG_ERR("ERROR TO WRITE CREDENTIALS SSID");
        return ret;
    }
    ret = this->_fs.write_buffer(PASSWORD_ID, this->credentials.password, pass_len);
    if (ret < 0)
    {
        LOG_ERR("ERROR TO WRITE CREDENTIALS PASSWORD");
        return ret;
    }

    return ret;
}