#pragma once
#include <stdint.h>

class IClock
{
public:
    virtual ~IClock() noexcept = default;

    virtual int sync_time() = 0;    // Function to sync wall-clock time
    virtual int get_week_day() = 0; // Function to parse and return the week day
    virtual int get_epoch() = 0;
    virtual int get_day() = 0;    // Function to parse and return the month day
    virtual int get_month() = 0;  // Function to parse and return the month
    virtual int get_year() = 0;   // Function to parse and return the year
    virtual int get_hour() = 0;   // Function to parse and return the hour
    virtual int get_minute() = 0; // Function to parse and return the minutes
};
