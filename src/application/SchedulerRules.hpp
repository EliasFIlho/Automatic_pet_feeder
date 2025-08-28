#pragma once

#include <zephyr/kernel.h>


typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
}SpecifcDateRule_t;

typedef struct{
    uint8_t hour;
    uint8_t minutes;
}TimeRule_t;

typedef enum{
    WEEKLY,
    SPECIF
}PeriodRule_t;


typedef struct SchedulerRules
{
    union
    {
        SpecifcDateRule_t date;
        uint8_t week_days;
    };
    TimeRule_t time;
    PeriodRule_t period;
    uint8_t amount;

} Rules_t;
