#ifndef SCHEDULE_BY_C_SCHEDULE_DATA_H
#define SCHEDULE_BY_C_SCHEDULE_DATA_H

#include <stddef.h>
#include <wchar.h>

#include "schedule.h"
#include "schedule_path.h"
#include "storage.h"

typedef enum {
    SCHEDULE_DATA_LOAD_OK,
    SCHEDULE_DATA_FILE_NOT_FOUND,
    SCHEDULE_DATA_INVALID_FORMAT,
    SCHEDULE_DATA_READ_ERROR,
    SCHEDULE_DATA_PATH_ERROR
} ScheduleDataLoadStatus;

typedef struct {
    ScheduleDataLoadStatus status;
    SchedulePathStatus pathStatus;
    StorageLoadResult storageResult;
} ScheduleDataLoadResult;

ScheduleDataLoadResult ScheduleData_Reload(ScheduleCollection *collection,
    wchar_t *resolvedPath, size_t resolvedPathSize);

#endif
