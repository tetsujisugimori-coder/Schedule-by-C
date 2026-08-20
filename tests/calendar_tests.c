#include <assert.h>
#include <stdio.h>

#include "calendar.h"

static void AssertMonthAssignment(int year, int month, int expectedFirstCell,
    int expectedDays)
{
    int cellDays[CALENDAR_GRID_CELL_COUNT];
    int cellIndex;

    Calendar_AssignMonthDays(year, month, cellDays);
    for (cellIndex = 0; cellIndex < CALENDAR_GRID_CELL_COUNT; ++cellIndex) {
        if (cellIndex < expectedFirstCell
            || cellIndex >= expectedFirstCell + expectedDays) {
            assert(cellDays[cellIndex] == 0);
        } else {
            assert(cellDays[cellIndex] == cellIndex - expectedFirstCell + 1);
        }
    }
}

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
    assert(Calendar_DaysInMonth(2026, 4) == 30);

    AssertMonthAssignment(2026, 2, 0, 28);
    AssertMonthAssignment(2024, 2, 4, 29);
    AssertMonthAssignment(2026, 4, 3, 30);
    AssertMonthAssignment(2026, 8, 6, 31);

    year = 2026;
    month = 1;
    Calendar_PreviousMonth(&year, &month);
    assert(year == 2025 && month == 12);

    year = 2026;
    month = 12;
    Calendar_NextMonth(&year, &month);
    assert(year == 2027 && month == 1);

    puts("Calendar date and 42-cell assignment tests passed.");
    return 0;
}
