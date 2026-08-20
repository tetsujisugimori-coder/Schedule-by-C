#include <assert.h>
#include <stdio.h>

#include "calendar.h"

int main(void)
{
    int year;
    int month;

    assert(Calendar_DaysInMonth(2026, 8) == 31);
    assert(Calendar_DaysInMonth(2026, 2) == 28);
    assert(Calendar_DaysInMonth(2024, 2) == 29);
    assert(Calendar_IsLeapYear(2024));
    assert(!Calendar_IsLeapYear(2026));
    assert(Calendar_DayOfWeek(2026, 8, 1) == 6);
    assert(Calendar_DayOfWeek(2026, 2, 1) == 0);

    year = 2026;
    month = 1;
    Calendar_PreviousMonth(&year, &month);
    assert(year == 2025 && month == 12);

    year = 2026;
    month = 12;
    Calendar_NextMonth(&year, &month);
    assert(year == 2027 && month == 1);

    puts("Calendar date calculation tests passed.");
    return 0;
}
