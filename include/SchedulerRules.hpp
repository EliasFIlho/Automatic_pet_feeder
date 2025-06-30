#pragma once


/*
* I won't win nothing by using these flags, in terms of space since i still have the SpecifDateRule_t inside of the union.
* But maybe i can use this for the JSON parser?
*/
#define SUNDAY_FLAG (1 << 0)
#define MONDAY_FLAG (1 << 1)
#define TUESDAY_FLAG (1 << 2)
#define WEDNESDAY_FLAG (1 << 3)
#define THURSDAY_FLAG (1 << 4)
#define FRIDAY_FLAG (1 << 5)
#define SATURDAY_FLAG (1 << 6)
#define DayFlag(x) (1 << x)



typedef struct
{
    uint8_t SUNDAY : 1;
    uint8_t MONDAY : 1;
    uint8_t TUESDAY : 1;
    uint8_t WEDNESDAY : 1;
    uint8_t THURSDAY : 1;
    uint8_t FRIDAY : 1;
    uint8_t SATURDAY : 1;
    uint8_t UNUSED : 1;

} WeekDaysRule_t;


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
        WeekDaysRule_t week_days;
    };
    TimeRule_t time;
    PeriodRule_t period;

} Rules_t;
