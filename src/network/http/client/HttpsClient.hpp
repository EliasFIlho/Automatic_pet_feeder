#pragma once

#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/kernel.h>


class HttpsClient
{
private:
    int sock;
    struct zsock_addrinfo *res;
    struct sockaddr_in addr;
    uint8_t recv_buf[512];
    struct k_thread HTTPSTask;

private:
    static void https_client_task(void *p1, void *, void *);
    void setup_tls_credentials();
    int setup_socket();
    int connect_socket();
    void close_socket();
    bool get_package();
public:
    void start_http();
    HttpsClient();
    ~HttpsClient();
};


