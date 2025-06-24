#pragma once

#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/kernel.h>

class HttpsClient
{
private:
    int socket;
    struct zsock_addrinfo *res;
    struct k_thread https_task;
    k_tid_t https_thread_id;
    uint8_t recv_buf[512];

private:
    int setup_socket();
    int connect_socket();
    void get_package();
    static void https_client_task(void *arg1, void *, void *);
public:
    HttpsClient();
    ~HttpsClient();
    void start_client_https();
};


