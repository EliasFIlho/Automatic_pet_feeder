#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/tls_credentials.h>
#include <string.h>
#include "HttpsClient.hpp"

#include "certificate.hpp"

#define HTTPS_REQUEST_HOST "raw.githubusercontent.com"
#define HTTPS_REQUEST_URL "/EliasFIlho/Automatic_pet_feeder/refs/heads/main/README.md"
#define HTTPS_REQUEST_PORT "443"
#define HTTPS_REQUEST_TIMEOUT 3000

#define HTTPS_THREAD_STACK_SIZE 8000
#define HTTPS_THREAD_PRIORITY 5
#define HTTPS_THREAD_OPTIONS (K_FP_REGS | K_ESSENTIAL)

K_THREAD_STACK_DEFINE(https_thread_stack_area, HTTPS_THREAD_STACK_SIZE);

#define HTTP_RECV_BUF_LEN 512

static int response_callback(struct http_response *resp,
                              enum http_final_call final_data,
                              void *user_data)
{
    char temp_buf[HTTP_RECV_BUF_LEN + 1];

    if (final_data == HTTP_DATA_MORE)
    {
        printk("Partial data received (%d bytes)\r\n", resp->data_len);
    }
    else if (final_data == HTTP_DATA_FINAL)
    {
        printk("All data received (%d bytes)\r\n", resp->data_len);
    }
    memcpy(temp_buf, resp->recv_buf, resp->data_len);
    temp_buf[resp->data_len] = '\0';
    printk("Received data:\r\n%s\r\n", temp_buf);
}

HttpsClient::HttpsClient()
{
}

HttpsClient::~HttpsClient()
{
}

int HttpsClient::setup_socket()
{
    int ret;
    struct zsock_addrinfo hints = {0};

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    ret = zsock_getaddrinfo(HTTPS_REQUEST_HOST, HTTPS_REQUEST_PORT, &hints, &this->res);
    if (ret != 0)
    {
        printk("Error[%d]: Could not get addr info from DNS host [%s] - %s\n\r", errno, HTTPS_REQUEST_HOST, strerror(errno));
        return ret;
    }
    else
    {
        printk("Socket addr info collected!!\n\r");
    }

    this->socket = zsock_socket(this->res->ai_family, this->res->ai_socktype, IPPROTO_TLS_1_3);
    if (this->socket < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}
int HttpsClient::connect_socket()
{
    int ret;
    ret = zsock_connect(this->socket, this->res->ai_addr, this->res->ai_addrlen);
    if (ret < 0)
    {
        printk("Error[%d]: Socket could not connect - %s\n\r", errno, strerror(errno));
        return ret;
    }
    else
    {
        printk("Socket connected!!\n\r");
    }
}

void HttpsClient::get_package()
{
    int ret;
    struct http_request req;
    memset(&req, 0, sizeof(req));
    req.method = HTTP_GET;
    req.url = HTTPS_REQUEST_URL;
    req.protocol = "HTTP/1.1";
    req.port = HTTPS_REQUEST_PORT;
    req.host = HTTPS_REQUEST_HOST;
    req.response = response_callback;
    req.recv_buf = this->recv_buf;
    req.recv_buf_len = sizeof(recv_buf);

    ret = http_client_req(socket, &req, HTTPS_REQUEST_TIMEOUT, NULL);
}

void HttpsClient::start_client_https()
{
    this->https_thread_id = k_thread_create(&this->https_task, https_thread_stack_area,
                                            K_THREAD_STACK_SIZEOF(https_thread_stack_area),
                                            https_client_task,
                                            this, NULL, NULL,
                                            HTTPS_THREAD_PRIORITY,
                                            HTTPS_THREAD_OPTIONS,K_NO_WAIT);
}

void HttpsClient::https_client_task(void *arg1, void *, void *)
{
    auto *client = static_cast<HttpsClient *>(arg1);
    client->setup_socket();
    client->connect_socket();

    while (1)
    {
        printk("Performing_request");
        client->get_package();
        k_msleep(500);
    }
}