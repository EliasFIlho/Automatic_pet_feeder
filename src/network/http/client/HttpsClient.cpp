#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <string.h>
#include "HttpsClient.hpp"
#include <zephyr/data/json.h>
#include "SchedulerRules.hpp"
#include "Storage.hpp"

#if !CONFIG_HTTP_VERSION

#include "certificate.hpp"
#include <zephyr/net/tls_credentials.h>

#endif

LOG_MODULE_REGISTER(HTTPS_CLIENT_LOG, CONFIG_LOG_DEFAULT_LEVEL);

#define HTTPS_THREAD_OPTIONS (K_FP_REGS | K_ESSENTIAL)
K_THREAD_STACK_DEFINE(HTTPS_STACK_AREA, CONFIG_HTTPS_THREAD_STACK_SIZE);

static const struct json_obj_descr rules_specific_date[] = {
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, year, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, month, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(SpecifcDateRule_t, day, JSON_TOK_UINT),
};

static const struct json_obj_descr rules_time[] = {
    JSON_OBJ_DESCR_PRIM(TimeRule_t, hour, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(TimeRule_t, minutes, JSON_TOK_UINT),
};

static const struct json_obj_descr rules[] = {

    JSON_OBJ_DESCR_OBJECT(Rules_t, date, rules_specific_date),
    JSON_OBJ_DESCR_OBJECT(Rules_t, time, rules_time),
    JSON_OBJ_DESCR_PRIM(Rules_t, period, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, week_days, JSON_TOK_UINT),
    JSON_OBJ_DESCR_PRIM(Rules_t, amount, JSON_TOK_UINT),

};

static int http_response_callback(struct http_response *resp, enum http_final_call final_data, void *user_data)
{
    char temp_buf[CONFIG_HTTP_RECV_BUF_LEN + 1];

    if (final_data == HTTP_DATA_MORE)
    {
        LOG_INF("Partial data received (%d bytes)\r\n", resp->data_len);
    }
    else if (final_data == HTTP_DATA_FINAL)
    {
        // TODO: Write the data structure received by server in filesystem
        LOG_INF("All data received (%d bytes)\r\n", resp->data_len);
        Rules_t rules_str;
        memcpy(temp_buf, resp->recv_buf, resp->data_len);
        temp_buf[resp->data_len] = '\0';

        // TODO: See if i can move this parser call to application and leave the callback just for read and store strings
        int ret = json_obj_parse(temp_buf, strlen(temp_buf), rules, ARRAY_SIZE(rules), &rules_str);
        if (ret < 0)
        {
            LOG_ERR("ERROR TO PARSE JSON OBJECT: %d", ret);
        }
        else
        {
            printk("-- DATA FROM JSON PARSED -- \r\n");

            // Storage &fs = Storage::getInstance();
            // fs.write_data(RULES_ID, rules_str);
        }
        LOG_INF("Received data:\r\n%s\r\n", temp_buf);
    }

    return 0;
}

static void print_addrinfo(struct zsock_addrinfo **results)
{
    char ipv4[INET_ADDRSTRLEN];
    struct sockaddr_in *sa;
    struct zsock_addrinfo *rp;

    // Iterate through the results
    for (rp = *results; rp != NULL; rp = rp->ai_next)
    {

        // Print IPv4 address
        if (rp->ai_addr->sa_family == AF_INET)
        {
            sa = (struct sockaddr_in *)rp->ai_addr;
            zsock_inet_ntop(AF_INET, &sa->sin_addr, ipv4, INET_ADDRSTRLEN);
            LOG_INF("IPv4: %s\r\n", ipv4);
        }
    }
}

HttpsClient::HttpsClient() : sock(-1)
{
}

HttpsClient::~HttpsClient()
{
}

int HttpsClient::setup_socket()
{
    int ret;
#if !CONFIG_HTTP_VERSION
    ret = tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, ca_certificate, sizeof(ca_certificate));
    if (ret != 0)
    {
        LOG_ERR("Unable to add tls credentials\r\n");
        return -1;
    }
    else
    {
        LOG_INF("TLS credentials added\r\n");
    }
#endif

    struct zsock_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    ret = zsock_getaddrinfo(CONFIG_HTTPS_REQUEST_ADDRESS, CONFIG_HTTPS_REQUEST_PORT, &hints, &this->res);

    
    if (ret != 0)
    {
        LOG_ERR("Unable to get server address\r\n");
        return -1;
    }
    else
    {
        print_addrinfo(&this->res);
    }
#if !CONFIG_HTTP_VERSION
    this->sock = zsock_socket(this->res->ai_family, this->res->ai_socktype, IPPROTO_TLS_1_2);
    if (this->sock < 0)
    {
        LOG_ERR("Socket fails to create -> socket fd = [%d]\r\n", this->sock);
        return -1;
    }
    else
    {
        LOG_INF("Socket created -> socket fd = [%d]\r\n", this->sock);
    }
#else
    this->sock = zsock_socket(this->res->ai_family, this->res->ai_socktype, this->res->ai_protocol);
    if (this->sock < 0)
    {
        LOG_ERR("Socket fails to create -> socket fd = [%d]\r\n", this->sock);
        return -1;
    }
    else
    {
        LOG_INF("Socket created -> socket fd = [%d]\r\n", this->sock);
        return 0;
    }
#endif

#if !CONFIG_HTTP_VERSION
    sec_tag_t sec_tag_opt[] = {
        CA_CERTIFICATE_TAG,
    };

    ret = zsock_setsockopt(this->sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt, sizeof(sec_tag_opt));
    if (ret != 0)
    {
        LOG_ERR("Unable to set socket option for TLS - [%d]\r\n", ret);
        return -1;
    }
    else
    {
        LOG_INF("Seted socket option for TLS - [%d]\r\n", ret);
    }

    ret = zsock_setsockopt(sock, SOL_TLS, TLS_HOSTNAME, CONFIG_HTTPS_REQUEST_ADDRESS, sizeof(CONFIG_HTTPS_REQUEST_ADDRESS));

    if (ret != 0)
    {
        LOG_ERR("Unable to set socket option HOST NAME for TLS - [%d]\r\n", ret);
        return -1;
    }
    else
    {
        LOG_INF("Seted socket option HOST NAME for TLS - [%d]\r\n", ret);
    }
    return 0;
#endif
}
int HttpsClient::connect_socket()
{
    int ret = zsock_connect(this->sock, this->res->ai_addr, this->res->ai_addrlen);
    if (ret != 0)
    {
        LOG_ERR("Error to connect socket[%d] - Error[%d]\r\n", this->sock, ret);
    }
    else
    {
        LOG_INF("Socket connected\r\n");
    }
    return ret;
}

bool HttpsClient::get_package()
{
    int ret;
    struct http_request req;
    memset(&req, 0, sizeof(req));
    req.method = HTTP_GET;
    req.url = CONFIG_HTTPS_REQUEST_URL;
    req.protocol = "HTTP/1.1";
    req.port = CONFIG_HTTPS_REQUEST_PORT;
    req.host = CONFIG_HTTPS_REQUEST_ADDRESS;
    req.response = http_response_callback;
    req.recv_buf = this->recv_buf;
    req.recv_buf_len = sizeof(this->recv_buf);
    ret = http_client_req(this->sock, &req, CONFIG_HTTPS_REQUEST_TIMEOUT, NULL);
    if (ret < 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}
void HttpsClient::close_socket()
{
    if (this->sock >= 0)
    {
        zsock_close(this->sock);
        this->sock = -1;
        LOG_INF("SOCKET CLOSED\n\r");
    }
}

void HttpsClient::https_client_task(void *p1, void *, void *)
{
    auto *self = static_cast<HttpsClient *>(p1);
    self->close_socket();

    self->setup_socket();

    while (true)
    {
        if (self->sock < 0)
        {
            LOG_WRN("SOCKET VALUE < 0: Socket will be setted again\n\r");

            self->setup_socket(); // Reinitialize socket and try again
        }
        self->connect_socket();
        self->get_package();
        self->close_socket();
        k_msleep(CONFIG_HTTPS_THREAD_PERIOD);
    }
}

void HttpsClient::start_http()
{
    k_thread_create(&this->HTTPSTask, HTTPS_STACK_AREA, CONFIG_HTTPS_THREAD_STACK_SIZE, this->https_client_task, this, NULL, NULL, CONFIG_HTTPS_THREAD_PRIORITY, HTTPS_THREAD_OPTIONS, K_NO_WAIT);
}