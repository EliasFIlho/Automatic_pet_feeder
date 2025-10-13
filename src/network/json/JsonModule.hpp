#pragma once

#include "IJson.hpp"

class JsonModule : public IJson
{
private:
    /* data */
public:
    JsonModule();
    ~JsonModule();
    int32_t parse(char *buffer_in, void *struct_out);
    int32_t encode(void *struct_in, char *buffer_out, size_t buf_len);
};
