#pragma once

#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/kernel.h>

#define CONFIG_HTTP_VERSION

class HttpsClient
{
private:
    int sock;
    struct zsock_addrinfo *res;
    uint8_t recv_buf[512];

private:
    static void https_client_task(void *, void *, void *);
    void setup_tls_credentials();
public:
    int setup_socket();
    int connect_socket();
    void get_package();
    HttpsClient();
    ~HttpsClient();
};


