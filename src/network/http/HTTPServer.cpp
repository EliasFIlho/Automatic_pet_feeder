#include "HTTPServer.hpp"
#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(HTTP_LOGS);

static uint16_t http_service_port = 80;

HTTP_SERVICE_DEFINE(http_service, NULL, &http_service_port, 1, 10, NULL, NULL, NULL);

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

struct http_resource_detail_static static_resource_detail = {
    .common = {
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
        .type = HTTP_RESOURCE_TYPE_STATIC,
        .content_encoding = "gzip",
        .content_type = "text/html"
    },
    .static_data = index_html_gz,
    .static_data_len = sizeof(index_html_gz)
};

HTTP_RESOURCE_DEFINE(static_resource, http_service, "/", &static_resource_detail);

HTTPServer::HTTPServer()
{
}

HTTPServer::~HTTPServer()
{
}

/*
TODO:

    Load static resources to ram
    Create JSON from struct
    Define Service
    Define Resources

*/
void HTTPServer::start()
{
    http_server_start();
    
}
void HTTPServer::stop()
{
    http_server_stop();
}