#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "schedule.h"

static void CopyText(char *destination, size_t destinationSize,
    const char *source)
{
    size_t length = strlen(source);

    assert(length < destinationSize);
    memcpy(destination, source, length + 1);
}

static Schedule MakeSchedule(const char *id, int year, int month, int day,
    const char *startTime)
{
    Schedule schedule = { 0 };

    CopyText(schedule.id, sizeof(schedule.id), id);
    schedule.year = year;
    schedule.month = month;
    schedule.day = day;
    CopyText(schedule.startTime, sizeof(schedule.startTime), startTime);
    CopyText(schedule.endTime, sizeof(schedule.endTime), "23:59");
    CopyText(schedule.title, sizeof(schedule.title), "test");
    CopyText(schedule.status, sizeof(schedule.status), "planned");
    CopyText(schedule.source, sizeof(schedule.source), "worklog");
    return schedule;
}

int main(void)
{
    ScheduleCollection collection;
    const Schedule *results[8];
    Schedule one = MakeSchedule("1", 2026, 8, 25, "13:00");
    Schedule two = MakeSchedule("2", 2026, 8, 25, "09:00");
    Schedule other = MakeSchedule("3", 2026, 8, 26, "18:00");
    Schedule duplicate = MakeSchedule("1", 2027, 1, 1, "08:00");
    Schedule sameTime = MakeSchedule("4", 2026, 8, 25, "09:00");
    Schedule emptyId = MakeSchedule("", 2026, 8, 25, "10:00");
    size_t count;

    ScheduleCollection_Init(&collection);
    assert(collection.count == 0);
    assert(ScheduleCollection_Add(&collection, &one));
    assert(ScheduleCollection_Add(&collection, &other));
    assert(!ScheduleCollection_Add(&collection, &duplicate));
    assert(!ScheduleCollection_Add(&collection, &emptyId));
    assert(collection.count == 2);

    assert(Schedule_HasForDate(&collection, 2026, 8, 25));
    assert(Schedule_HasForDate(&collection, 2026, 8, 26));
    assert(!Schedule_HasForDate(&collection, 2026, 8, 27));

    count = Schedule_GetForDate(&collection, 2026, 8, 25, results, 8);
    assert(count == 1);
    assert(strcmp(results[0]->id, "1") == 0);

    assert(ScheduleCollection_Add(&collection, &two));
    assert(ScheduleCollection_Add(&collection, &sameTime));
    count = Schedule_GetForDate(&collection, 2026, 8, 25, results, 8);
    assert(count == 3);
    assert(strcmp(results[0]->startTime, "09:00") == 0);
    assert(strcmp(results[1]->startTime, "09:00") == 0);
    assert(strcmp(results[0]->id, "2") == 0);
    assert(strcmp(results[1]->id, "4") == 0);
    assert(strcmp(results[2]->startTime, "13:00") == 0);

    count = Schedule_GetForDate(&collection, 2026, 8, 27, results, 8);
    assert(count == 0);

    puts("Schedule search and sort tests passed.");
    return 0;
}
