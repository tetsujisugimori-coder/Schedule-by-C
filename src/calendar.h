#ifndef SCHEDULE_BY_C_CALENDAR_H
#define SCHEDULE_BY_C_CALENDAR_H

int Calendar_IsLeapYear(int year);
int Calendar_DaysInMonth(int year, int month);
int Calendar_DayOfWeek(int year, int month, int day);
void Calendar_PreviousMonth(int *year, int *month);
void Calendar_NextMonth(int *year, int *month);

#endif
