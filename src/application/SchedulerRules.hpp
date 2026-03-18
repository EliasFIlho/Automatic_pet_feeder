#pragma once

#include <stdint.h>
#include "IStorage.hpp"
#include "ISchedulerRules.hpp"
#include <bitset>
#include "array"


#define RULES_ID_BASE RULES_ID_1
#define RULES_ID_END RULES_ID_5

class SchedulerRules : public ISchedulerRules
{
private:

    IStorage &_fs;
    std::bitset<CONFIG_MAX_SCHEDULER_RULES> map;
public:
    void init();
    int32_t write_rule(void *ptr, size_t size);
    int32_t read_rules(std::array<Scheduled_Rule_t, CONFIG_MAX_SCHEDULER_RULES> &rules);
    int32_t delete_rule(uint8_t rule_fs_idx);
    uint8_t get_number_of_rules();
    uint32_t clear_rules();

    SchedulerRules(IStorage &fs);
    ~SchedulerRules();
};
