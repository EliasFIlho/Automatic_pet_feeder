#pragma once
#include <stdint.h>

enum{
    SSID_ID,
    PASSWORD_ID,
    RULES_ID
};

enum class FILE_SYSTEM_ERROR
{
    STORAGE_ERROR_MOUNT,
    STORAGE_ERROR_PAGE_INFO,
    STORAGE_ERROR_DEVICE,
    STORAGE_ERROR_NOT_ENOUTH_SPACE,
    STORAGE_ERROR_CHECK_FREE_SPACE,
    STORAGE_OK
};


class IStorage
{
public:

    virtual ~IStorage() noexcept = default;
    virtual FILE_SYSTEM_ERROR init_storage() = 0;
    virtual int read_data(uint32_t id, char *buf, unsigned int buf_len) = 0;
    virtual int32_t write_data(uint32_t id, const char *str) = 0;
};
