#pragma once

#include <stdint.h>

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
    /*TODO: Maybe is a good idea to turn TimeRule_t struct into a fixed size vector, so the client can set more the then just one time data 
    e.g. All weeks day at 09AM, 3PM, 9PM */
    TimeRule_t time;
    PeriodRule_t period;
    uint8_t week_days;
    uint8_t amount;

} Rules_t;
