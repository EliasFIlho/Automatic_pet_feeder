#pragma once

#include "IJson.hpp"

class JsonModule : public IJson
{
private:
    /* data */
public:
    JsonModule();
    ~JsonModule();
    void parse(char *buffer_in, void *struct_out);
    void encode(void *struct_in, char *buffer_out);
};
