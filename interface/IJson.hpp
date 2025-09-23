#pragma once

#include <stdint.h>

class IJson
{

public:
    virtual ~IJson() noexcept = default;
    virtual void parse(char * buffer_in, void * struct_out) = 0;
    virtual void encode(void * struct_in,char * buffer_out) = 0;
};

