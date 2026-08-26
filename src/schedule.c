#include "schedule.h"

#include <string.h>

static int IsSameDate(const Schedule *schedule, int year, int month, int day)
{
    return schedule->year == year
        && schedule->month == month
        && schedule->day == day;
}

static int CompareByStartTime(const Schedule *left, const Schedule *right)
{
    int timeOrder = strcmp(left->startTime, right->startTime);

    if (timeOrder != 0) {
        return timeOrder;
    }

    return strcmp(left->id, right->id);
}

void ScheduleCollection_Init(ScheduleCollection *collection)
{
    if (collection != NULL) {
        collection->count = 0;
    }
}

int ScheduleCollection_Add(ScheduleCollection *collection,
    const Schedule *schedule)
{
    size_t index;

    if (collection == NULL || schedule == NULL || schedule->id[0] == '\0'
        || collection->count >= MAX_SCHEDULES) {
        return 0;
    }

    for (index = 0; index < collection->count; ++index) {
        if (strcmp(collection->items[index].id, schedule->id) == 0) {
            return 0;
        }
    }

    collection->items[collection->count] = *schedule;
    collection->count++;
    return 1;
}

int Schedule_HasForDate(const ScheduleCollection *collection,
    int year, int month, int day)
{
    size_t index;

    if (collection == NULL) {
        return 0;
    }

    for (index = 0; index < collection->count; ++index) {
        if (IsSameDate(&collection->items[index], year, month, day)) {
            return 1;
        }
    }

    return 0;
}

size_t Schedule_GetForDate(const ScheduleCollection *collection,
    int year, int month, int day, const Schedule *results[],
    size_t resultsCapacity)
{
    size_t index;
    size_t resultCount = 0;

    if (collection == NULL || results == NULL || resultsCapacity == 0) {
        return 0;
    }

    for (index = 0; index < collection->count && resultCount < resultsCapacity;
        ++index) {
        const Schedule *candidate = &collection->items[index];
        size_t insertAt;

        if (!IsSameDate(candidate, year, month, day)) {
            continue;
        }

        insertAt = resultCount;
        while (insertAt > 0
            && CompareByStartTime(candidate, results[insertAt - 1]) < 0) {
            results[insertAt] = results[insertAt - 1];
            insertAt--;
        }
        results[insertAt] = candidate;
        resultCount++;
    }

    return resultCount;
}
