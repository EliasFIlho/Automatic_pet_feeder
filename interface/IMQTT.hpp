#pragma once

class IMQTT
{
public:
    virtual ~IMQTT() noexcept = default;
    virtual void start_mqtt() = 0;
    virtual void block_mqtt() = 0;
    virtual void release_mqtt() = 0;
    virtual void abort() = 0;
};
