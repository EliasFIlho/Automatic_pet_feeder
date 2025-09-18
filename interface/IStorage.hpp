#pragma once
#include <stdint.h>

enum{
    SSID_ID,
    PASSWORD_ID,
    RULES_ID
};


class IStorage
{
public:

    virtual ~IStorage() noexcept = default;
    virtual int init_storage() = 0;
    virtual int read_data(uint32_t id, char *buf, unsigned int buf_len) = 0;
    virtual int write_data(uint32_t id, const char *str) = 0;
};
