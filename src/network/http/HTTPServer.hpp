#pragma once

#include "IHTTPServer.hpp"

class HTTPServer : public IHTTPServer
{
private:
    /* data */
public:
    void start();
    void stop();
    HTTPServer(/* args */);
    ~HTTPServer();
};
