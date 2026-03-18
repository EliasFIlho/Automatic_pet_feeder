#pragma once
#include <stdint.h>
#include <string.h>

enum
{
    SSID_ID,
    PASSWORD_ID,
    RULES_ID_1,
    RULES_ID_2,
    RULES_ID_3,
    RULES_ID_4,
    RULES_ID_5
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
    virtual size_t read_buffer(uint32_t id, void *ptr, size_t size) = 0;
    virtual int32_t write_buffer(uint32_t id, void *ptr, size_t size) = 0;
    virtual bool is_id_empty(uint32_t id) = 0;
    virtual int32_t delete_data(uint32_t id) = 0;
};
