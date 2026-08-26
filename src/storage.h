#ifndef SCHEDULE_BY_C_STORAGE_H
#define SCHEDULE_BY_C_STORAGE_H

#include <stddef.h>
#include <wchar.h>

#include "schedule.h"

#define SCHEDULE_CSV_HEADER \
    "id,date,start_time,end_time,title,note,status,source"

typedef enum {
    STORAGE_LOAD_OK,
    STORAGE_LOAD_FILE_NOT_FOUND,
    STORAGE_LOAD_READ_ERROR
} StorageLoadStatus;

typedef struct {
    StorageLoadStatus status;
    size_t loadedCount;
    size_t skippedCount;
} StorageLoadResult;

StorageLoadResult Storage_LoadSchedules(const wchar_t *filePath,
    ScheduleCollection *collection);

#endif
