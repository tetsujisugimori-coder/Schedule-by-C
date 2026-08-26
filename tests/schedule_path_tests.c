#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <windows.h>
#include <shlobj.h>

#include "schedule_path.h"

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

static void TestEnvironmentPathHasPriority(void)
{
    const wchar_t *expected = L"C:\\temporary schedule\\shared.csv";
    wchar_t actual[SCHEDULE_CSV_PATH_CAPACITY];

    assert(_wputenv_s(SCHEDULE_CSV_PATH_ENV, expected) == 0);
    assert(SchedulePath_Resolve(actual,
        sizeof(actual) / sizeof(actual[0])) == SCHEDULE_PATH_OK);
    assert(wcscmp(actual, expected) == 0);
    assert(SchedulePath_Resolve(actual, 4) == SCHEDULE_PATH_BUFFER_TOO_SMALL);
}

static void TestDocumentsDefaultPath(void)
{
    wchar_t actual[SCHEDULE_CSV_PATH_CAPACITY];
    PWSTR documents = NULL;
    size_t documentsLength;
    const wchar_t *suffix;

    assert(_wputenv_s(SCHEDULE_CSV_PATH_ENV, L"") == 0);
    assert(SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Documents,
        KF_FLAG_DEFAULT, NULL, &documents)));
    assert(documents != NULL);
    assert(SchedulePath_Resolve(actual,
        sizeof(actual) / sizeof(actual[0])) == SCHEDULE_PATH_OK);

    documentsLength = wcslen(documents);
    assert(wcsncmp(actual, documents, documentsLength) == 0);
    suffix = actual + documentsLength;
    if (documentsLength > 0 && documents[documentsLength - 1] != L'\\'
        && documents[documentsLength - 1] != L'/') {
        assert(*suffix++ == L'\\');
    }
    assert(wcscmp(suffix, L"ScheduleData\\schedule.csv") == 0);
    CoTaskMemFree(documents);
}

int main(void)
{
    SavedEnvironment saved = SaveEnvironment();

    TestEnvironmentPathHasPriority();
    TestDocumentsDefaultPath();
    RestoreEnvironment(&saved);

    puts("Schedule path tests passed.");
    return 0;
}
