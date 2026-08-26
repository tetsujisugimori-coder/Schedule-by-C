#ifndef SCHEDULE_BY_C_SCHEDULE_H
#define SCHEDULE_BY_C_SCHEDULE_H

#include <stddef.h>

#define MAX_SCHEDULES 1024
#define MAX_ID_LENGTH 64
#define MAX_TIME_LENGTH 5
#define MAX_TITLE_LENGTH 128
#define MAX_NOTE_LENGTH 256
#define MAX_STATUS_LENGTH 32
#define MAX_SOURCE_LENGTH 32

typedef struct {
    char id[MAX_ID_LENGTH + 1];
    int year;
    int month;
    int day;
    char startTime[MAX_TIME_LENGTH + 1];
    char endTime[MAX_TIME_LENGTH + 1];
    char title[MAX_TITLE_LENGTH + 1];
    char note[MAX_NOTE_LENGTH + 1];
    char status[MAX_STATUS_LENGTH + 1];
    char source[MAX_SOURCE_LENGTH + 1];
} Schedule;

typedef struct {
    Schedule items[MAX_SCHEDULES];
    size_t count;
} ScheduleCollection;

void ScheduleCollection_Init(ScheduleCollection *collection);
int ScheduleCollection_Add(ScheduleCollection *collection,
    const Schedule *schedule);
int Schedule_HasForDate(const ScheduleCollection *collection,
    int year, int month, int day);
size_t Schedule_GetForDate(const ScheduleCollection *collection,
    int year, int month, int day, const Schedule *results[],
    size_t resultsCapacity);

#endif
