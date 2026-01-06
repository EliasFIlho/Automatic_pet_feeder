#pragma once
class IHTTPServer
{
public:
    virtual ~IHTTPServer() {};
    virtual void start() = 0;
    virtual void stop() = 0;
};