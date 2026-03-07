#pragma once
#include <stdint.h>
#include "types.hpp"
#include "array"
class ISchedulerRules
{

public:
    virtual ~ISchedulerRules() noexcept = default;
    virtual void init() = 0;
    virtual int32_t write_rule(void *ptr, size_t size) = 0;
    virtual void read_rules(std::array<Scheduled_Rule_t, CONFIG_MAX_SCHEDULER_RULES> &rules) = 0;
    virtual void delete_rule() = 0;
    virtual uint8_t get_number_of_rules() = 0;
};
