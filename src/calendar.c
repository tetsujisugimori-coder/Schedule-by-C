#include "calendar.h"

int Calendar_IsLeapYear(int year)
{
    return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

int Calendar_DaysInMonth(int year, int month)
{
    static const int days[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    if (month == 2) {
        return days[1] + Calendar_IsLeapYear(year);
    }

    return days[month - 1];
}

int Calendar_DayOfWeek(int year, int month, int day)
{
    static const int monthOffsets[] = {
        0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4
    };

    /* January and February are treated as months 13 and 14 of the prior year. */
    if (month < 3) {
        year--;
    }

    return (year + year / 4 - year / 100 + year / 400
        + monthOffsets[month - 1] + day) % 7;
}

void Calendar_PreviousMonth(int *year, int *month)
{
    (*month)--;
    if (*month == 0) {
        *month = 12;
        (*year)--;
    }
}

void Calendar_NextMonth(int *year, int *month)
{
    (*month)++;
    if (*month == 13) {
        *month = 1;
        (*year)++;
    }
}
