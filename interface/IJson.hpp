#pragma once

#include <stdint.h>
#include <string.h>
class IJson
{

public:
    virtual ~IJson() noexcept = default;
    virtual int32_t parse(char * buffer_in, void * struct_out) = 0;
    virtual int32_t encode(void *struct_in, char *buffer_out, size_t buf_len) = 0;
};

