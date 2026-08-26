#include "schedule_data.h"

#include <errno.h>
#include <stdlib.h>

ScheduleDataLoadResult ScheduleData_Reload(ScheduleCollection *collection,
    wchar_t *resolvedPath, size_t resolvedPathSize)
{
    ScheduleDataLoadResult result = {
        SCHEDULE_DATA_PATH_ERROR,
        SCHEDULE_PATH_INVALID_ARGUMENT,
        { STORAGE_LOAD_READ_ERROR, 0, 0, 0 }
    };
    ScheduleCollection *loadedSchedules;

    if (collection == NULL || resolvedPath == NULL || resolvedPathSize == 0) {
        return result;
    }

    result.pathStatus = SchedulePath_Resolve(resolvedPath, resolvedPathSize);
    if (result.pathStatus != SCHEDULE_PATH_OK) {
        return result;
    }

    loadedSchedules = (ScheduleCollection *)malloc(sizeof(*loadedSchedules));
    if (loadedSchedules == NULL) {
        result.status = SCHEDULE_DATA_READ_ERROR;
        result.storageResult.errorNumber = ENOMEM;
        return result;
    }

    result.storageResult = Storage_LoadSchedules(resolvedPath,
        loadedSchedules);
    switch (result.storageResult.status) {
    case STORAGE_LOAD_OK:
        *collection = *loadedSchedules;
        result.status = SCHEDULE_DATA_LOAD_OK;
        break;
    case STORAGE_LOAD_FILE_NOT_FOUND:
        result.status = SCHEDULE_DATA_FILE_NOT_FOUND;
        break;
    case STORAGE_LOAD_INVALID_FORMAT:
        result.status = SCHEDULE_DATA_INVALID_FORMAT;
        break;
    case STORAGE_LOAD_READ_ERROR:
    default:
        result.status = SCHEDULE_DATA_READ_ERROR;
        break;
    }

    free(loadedSchedules);

    return result;
}
