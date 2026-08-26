#ifndef SCHEDULE_BY_C_SCHEDULE_PATH_H
#define SCHEDULE_BY_C_SCHEDULE_PATH_H

#include <stddef.h>
#include <wchar.h>

#define SCHEDULE_CSV_PATH_ENV L"SCHEDULE_CSV_PATH"
#define SCHEDULE_CSV_PATH_CAPACITY 32768

typedef enum {
    SCHEDULE_PATH_OK,
    SCHEDULE_PATH_INVALID_ARGUMENT,
    SCHEDULE_PATH_BUFFER_TOO_SMALL,
    SCHEDULE_PATH_DOCUMENTS_UNAVAILABLE
} SchedulePathStatus;

SchedulePathStatus SchedulePath_Resolve(wchar_t *buffer, size_t bufferSize);
const wchar_t *SchedulePath_StatusMessage(SchedulePathStatus status);

#endif
