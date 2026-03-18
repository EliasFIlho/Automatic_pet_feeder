#include "SchedulerRules.hpp"

/**
 * @brief Init rules controller and RAM with the current filesystem data at startup
 *
 */
void SchedulerRules::init()
{
    this->map.reset();
    for (uint32_t i = 0; i < CONFIG_MAX_SCHEDULER_RULES; i++)
    {
        uint32_t rule_id = RULES_ID_BASE + i;
        if (!this->_fs.is_id_empty(rule_id))
        {
            this->map.set(i, true);
        }
    }
}

/**
 * @brief Write a buffer in the first avaliable slot in filesystem
 *
 * @param ptr Data pointer
 * @param size Data sizer
 * @return int32_t return from Storage::write_buffer
 */
int32_t SchedulerRules::write_rule(void *ptr, size_t size)
{
    if (map.count() >= CONFIG_MAX_SCHEDULER_RULES)
    {
        return 0; // Rules full
    }

    for (uint32_t i = 0; i < CONFIG_MAX_SCHEDULER_RULES; i++)
    {
        if (!this->map.test(i))
        {
            uint32_t rule_id = RULES_ID_BASE + i;
            int ret = this->_fs.write_buffer(rule_id, ptr, size);
            if (ret > 0)
            {
                this->map.set(i, true);
                return ret;
            }
        }
    }
    return 0; // Could not write due to Rules full
}

/**
 * @brief Update the RAM buffer with the filesystem data
 * TODO: Add return value to validate reads
 */
int32_t SchedulerRules::read_rules(std::array<Scheduled_Rule_t, CONFIG_MAX_SCHEDULER_RULES> &rules)
{
    if (this->map.count() <= 0)
    {
        return 1; // No data stored
    }
    uint8_t idx = 0;
    for (uint32_t i = 0; i < CONFIG_MAX_SCHEDULER_RULES; i++)
    {
        if (this->map.test(i))
        {
            uint32_t rule_id = RULES_ID_BASE + i;
            int32_t ret = this->_fs.read_buffer(rule_id, &rules[idx].rule, sizeof(rules[idx].rule));
            if (ret > 0)
            {
                rules[idx].state = RULE_STATE::READY;
                rules[idx].fs_index = i;
                idx++;
            }
            else
            {
                return ret;
            }
        }
    }
    return 0;
}

int32_t SchedulerRules::delete_rule(uint8_t rule_fs_idx)
{
    uint32_t rule_id = rule_fs_idx + RULES_ID_BASE;
    int32_t ret = this->_fs.delete_data(rule_id);
    if (ret == 0)
    {
        this->map.set(rule_fs_idx, false);
    }
    return ret;
}

uint32_t SchedulerRules::clear_rules()
{
    for (uint32_t i = 0; i < CONFIG_MAX_SCHEDULER_RULES; i++)
    {
        int32_t ret = this->_fs.delete_data(RULES_ID_BASE + i);
        if (ret != 0) {
            return ret;
        }
        this->map.set(i, false);
    }
    return 0;
}

uint8_t SchedulerRules::get_number_of_rules()
{
    return static_cast<uint8_t>(this->map.count());
}

SchedulerRules::SchedulerRules(IStorage &fs) : _fs{fs}
{
}

SchedulerRules::~SchedulerRules() {}