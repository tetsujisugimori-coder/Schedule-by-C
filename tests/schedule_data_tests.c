#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <windows.h>

#include "schedule_data.h"

typedef struct {
    int wasSet;
    wchar_t value[SCHEDULE_CSV_PATH_CAPACITY];
} SavedEnvironment;

static SavedEnvironment SaveEnvironment(void)
{
    SavedEnvironment saved = { 0, { 0 } };
    DWORD length = GetEnvironmentVariableW(SCHEDULE_CSV_PATH_ENV, saved.value,
        SCHEDULE_CSV_PATH_CAPACITY);

    assert(length < SCHEDULE_CSV_PATH_CAPACITY);
    saved.wasSet = length > 0;
    return saved;
}

static void RestoreEnvironment(const SavedEnvironment *saved)
{
    assert(_wputenv_s(SCHEDULE_CSV_PATH_ENV,
        saved->wasSet ? saved->value : L"") == 0);
}

static void CreateTemporaryDirectory(wchar_t directory[MAX_PATH])
{
    wchar_t temporaryRoot[MAX_PATH];
    DWORD rootLength = GetTempPathW(MAX_PATH, temporaryRoot);

    assert(rootLength > 0 && rootLength < MAX_PATH);
    assert(GetTempFileNameW(temporaryRoot, L"sbc", 0, directory) != 0);
    assert(DeleteFileW(directory));
    assert(CreateDirectoryW(directory, NULL));
}

static void WriteCsv(const wchar_t *path, const char *content)
{
    FILE *file = NULL;

    assert(_wfopen_s(&file, path, L"wb") == 0);
    assert(file != NULL);
    assert(fputs(content, file) >= 0);
    assert(fclose(file) == 0);
}

static void TestMissingReloadAndAtomicReplacement(const wchar_t *csvPath,
    const wchar_t *temporaryDirectory)
{
    ScheduleCollection schedules;
    ScheduleDataLoadResult result;
    wchar_t resolvedPath[SCHEDULE_CSV_PATH_CAPACITY];

    ScheduleCollection_Init(&schedules);
    result = ScheduleData_Reload(&schedules, resolvedPath,
        sizeof(resolvedPath) / sizeof(resolvedPath[0]));
    assert(result.status == SCHEDULE_DATA_FILE_NOT_FOUND);
    assert(schedules.count == 0);
    assert(wcscmp(resolvedPath, csvPath) == 0);

    WriteCsv(csvPath, SCHEDULE_CSV_HEADER "\n"
        "1,2026-08-25,09:00,10:00,First,note,planned,worklog\n");
    result = ScheduleData_Reload(&schedules, resolvedPath,
        sizeof(resolvedPath) / sizeof(resolvedPath[0]));
    assert(result.status == SCHEDULE_DATA_LOAD_OK);
    assert(result.storageResult.loadedCount == 1);
    assert(schedules.count == 1);
    assert(strcmp(schedules.items[0].id, "1") == 0);

    WriteCsv(csvPath, SCHEDULE_CSV_HEADER "\n"
        "1,2026-08-25,09:00,10:00,First,note,planned,worklog\n"
        "2,2026-08-25,11:00,12:00,Added,note,planned,worklog\n");
    result = ScheduleData_Reload(&schedules, resolvedPath,
        sizeof(resolvedPath) / sizeof(resolvedPath[0]));
    assert(result.status == SCHEDULE_DATA_LOAD_OK);
    assert(schedules.count == 2);
    assert(strcmp(schedules.items[1].id, "2") == 0);

    WriteCsv(csvPath, "wrong,header\n"
        "3,2026-08-25,13:00,14:00,Bad reload,note,planned,worklog\n");
    result = ScheduleData_Reload(&schedules, resolvedPath,
        sizeof(resolvedPath) / sizeof(resolvedPath[0]));
    assert(result.status == SCHEDULE_DATA_INVALID_FORMAT);
    assert(schedules.count == 2);
    assert(strcmp(schedules.items[0].id, "1") == 0);
    assert(strcmp(schedules.items[1].id, "2") == 0);

    assert(_wputenv_s(SCHEDULE_CSV_PATH_ENV, temporaryDirectory) == 0);
    result = ScheduleData_Reload(&schedules, resolvedPath,
        sizeof(resolvedPath) / sizeof(resolvedPath[0]));
    assert(result.status == SCHEDULE_DATA_READ_ERROR);
    assert(result.storageResult.errorNumber != 0);
    assert(schedules.count == 2);
    assert(strcmp(schedules.items[0].id, "1") == 0);
    assert(strcmp(schedules.items[1].id, "2") == 0);
    assert(_wputenv_s(SCHEDULE_CSV_PATH_ENV, csvPath) == 0);
}

int main(void)
{
    SavedEnvironment saved = SaveEnvironment();
    wchar_t temporaryDirectory[MAX_PATH];
    wchar_t csvPath[MAX_PATH];

    CreateTemporaryDirectory(temporaryDirectory);
    assert(swprintf_s(csvPath, MAX_PATH, L"%ls\\schedule.csv",
        temporaryDirectory) > 0);
    assert(_wputenv_s(SCHEDULE_CSV_PATH_ENV, csvPath) == 0);

    TestMissingReloadAndAtomicReplacement(csvPath, temporaryDirectory);

    assert(DeleteFileW(csvPath));
    assert(RemoveDirectoryW(temporaryDirectory));
    RestoreEnvironment(&saved);
    puts("Schedule data reload tests passed.");
    return 0;
}
