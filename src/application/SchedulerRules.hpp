#pragma once

#include <stdint.h>

#define MAX_WEEK_DAYS_MASK_VALUE 0x7F

#define MAX_DAYS_SPECIFIC_VALUE 31
#define MAX_MONTHS_SPECIFIC_VALUE 12
#define MIN_DAYS_SPECIFIC_VALUE 1
#define MIN_MONTHS_SPECIFIC_VALUE 1

#define MAX_HOUR_TIME_VALUE 23
#define MAX_MINUTE_TIME_VALUE 59



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

typedef struct SchedulerRules
{
    SpecifcDateRule_t date;
    TimeRule_t time;
    PeriodRule_t period;
    uint8_t week_days;
    uint8_t amount;

} Rules_t;
