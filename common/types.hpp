#pragma once

#include <stdint.h>
#include "Enums.hpp"
#include <bitset>
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
} SpecifcDateRule_t;

typedef struct
{
    uint8_t hour;
    uint8_t minutes;
} TimeRule_t;

typedef enum
{
    WEEKLY,
    SPECIF
} PeriodRule_t;

/**
 * @brief Struct to define the set of variables that makes a scheduled operation
 *
 */

typedef struct
{
    SpecifcDateRule_t date;
    TimeRule_t time;
    PeriodRule_t period;
    uint8_t week_days;
    uint8_t amount;

} Rules_t;



/**
 * @brief Struct that represent a scheduled_rule and his execution status
 * 
 * The expected usage for this struct is to application check the vales in the rule field and perform the operation
 * once operation is done check the flag
 * 
 * if the rule.period field is WEEKLY, application needs to unset WasDispenserExecuted flag, so he can operate again; 
 * 
 */
typedef struct
{
    Rules_t rule;
    RULE_STATE state;
    uint8_t fs_index;

} Scheduled_Rule_t;