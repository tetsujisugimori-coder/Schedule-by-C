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

    if (month < 1 || month > 12) {
        return 0;
    }

    if (month == 2) {
        return days[1] + Calendar_IsLeapYear(year);
    }

    return days[month - 1];
}

int Calendar_IsValidDate(int year, int month, int day)
{
    int daysInMonth;

    if (year < 1) {
        return 0;
    }

    daysInMonth = Calendar_DaysInMonth(year, month);
    return day >= 1 && day <= daysInMonth;
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

void Calendar_AssignMonthDays(int year, int month,
    int cellDays[CALENDAR_GRID_CELL_COUNT])
{
    int firstWeekday = Calendar_DayOfWeek(year, month, 1);
    int daysInMonth = Calendar_DaysInMonth(year, month);
    int cellIndex;
    int day;

    for (cellIndex = 0; cellIndex < CALENDAR_GRID_CELL_COUNT; ++cellIndex) {
        cellDays[cellIndex] = 0;
    }

    for (day = 1; day <= daysInMonth; ++day) {
        cellDays[firstWeekday + day - 1] = day;
    }
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
