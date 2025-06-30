#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/tls_credentials.h>
#include <string.h>
#include "HttpsClient.hpp"

#ifndef CONFIG_HTTP

#include "certificate.hpp"
#define HTTPS_REQUEST_PORT "443"

#else

#define HTTPS_REQUEST_PORT "80"

#endif

#define HTTPS_REQUEST_HOST "google.com"
#define HTTPS_REQUEST_URL "/"

#define HTTPS_REQUEST_TIMEOUT 3000

#define HTTPS_THREAD_STACK_SIZE 8000
#define HTTPS_THREAD_PRIORITY 5
#define HTTPS_THREAD_OPTIONS (K_FP_REGS | K_ESSENTIAL)

#define HTTP_RECV_BUF_LEN 512

static int http_response_callback(struct http_response *resp, enum http_final_call final_data, void *user_data)
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
            printk("IPv4: %s\r\n", ipv4);
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
#ifndef CONFIG_HTTP
    ret = tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, ca_certificate, sizeof(ca_certificate));
    if (ret != 0)
    {
        printk("Unable to add tls credentials\r\n");
        return -1;
    }
    else
    {
        printk("TLS credentials added\r\n");
    }
#endif

    struct zsock_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    ret = zsock_getaddrinfo(HTTPS_REQUEST_HOST, HTTPS_REQUEST_PORT, &hints, &this->res);
    if (ret != 0)
    {
        printk("Unable to get server address\r\n");
        return -1;
    }
    else
    {
        print_addrinfo(&this->res);
    }
#ifndef CONFIG_HTTP
    this->sock = zsock_socket(this->res->ai_family, this->res->ai_socktype, IPPROTO_TLS_1_2);
    if (this->sock < 0)
    {
        printk("Socket fails to create -> socket fd = [%d]\r\n", this->sock);
        return -1;
    }
    else
    {
        printk("Socket created -> socket fd = [%d]\r\n", this->sock);
    }
#else
    this->sock = zsock_socket(this->res->ai_family, this->res->ai_socktype, this->res->ai_protocol);
    if (this->sock < 0)
    {
        printk("Socket fails to create -> socket fd = [%d]\r\n", this->sock);
        return -1;
    }
    else
    {
        printk("Socket created -> socket fd = [%d]\r\n", this->sock);
        return 0;
    }
#endif

#ifndef CONFIG_HTTP
    sec_tag_t sec_tag_opt[] = {
        CA_CERTIFICATE_TAG,
    };

    ret = zsock_setsockopt(this->sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt, sizeof(sec_tag_opt));
    if (ret != 0)
    {
        printk("Unable to set socket option for TLS - [%d]\r\n", ret);
        return -1;
    }
    else
    {
        printk("Seted socket option for TLS - [%d]\r\n", ret);
    }

    ret = zsock_setsockopt(sock, SOL_TLS, TLS_HOSTNAME, HTTPS_REQUEST_HOST, sizeof(HTTPS_REQUEST_HOST));

    if (ret != 0)
    {
        printk("Unable to set socket option HOST NAME for TLS - [%d]\r\n", ret);
        return -1;
    }
    else
    {
        printk("Seted socket option HOST NAME for TLS - [%d]\r\n", ret);
    }
    return 0;
#endif
}
int HttpsClient::connect_socket()
{
    int ret = zsock_connect(this->sock, this->res->ai_addr, this->res->ai_addrlen);
    if (ret != 0)
    {
        printk("Error to connect socket[%d] - Error[%d]\r\n", this->sock, ret);
    }
    else
    {
        printk("Socket connected\r\n");
    }
    return ret;
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
    req.response = http_response_callback;
    req.recv_buf = this->recv_buf;
    req.recv_buf_len = sizeof(this->recv_buf);
    printk("Request struct populated\r\n");

    ret = http_client_req(this->sock, &req, HTTPS_REQUEST_TIMEOUT, NULL);
}

void HttpsClient::https_client_task(void *, void *, void *)
{
}