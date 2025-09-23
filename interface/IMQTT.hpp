#pragma once

class IMQTT
{
public:
    virtual ~IMQTT() noexcept = default;
    virtual void start_mqtt() = 0;
    virtual void abort() = 0;
    virtual bool is_payload_updated() = 0;
};
