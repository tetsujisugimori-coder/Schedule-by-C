#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage.h"

#define TEST_FILE_PATH L"storage_test_schedule.csv"
#define MISSING_FILE_PATH L"storage_test_missing.csv"
#define UNICODE_FILE_PATH L"予定 データ 😀 storage.csv"
#define MISSING_UNICODE_FILE_PATH L"存在しない 予定 😀.csv"

static int RemoveTestFile(const wchar_t *filePath)
{
    return _wremove(filePath);
}

static void CleanupTestFiles(void)
{
    RemoveTestFile(TEST_FILE_PATH);
    RemoveTestFile(MISSING_FILE_PATH);
    RemoveTestFile(UNICODE_FILE_PATH);
    RemoveTestFile(MISSING_UNICODE_FILE_PATH);
}

static FILE *OpenTestFile(const wchar_t *filePath)
{
    FILE *file = NULL;

#ifdef _MSC_VER
    assert(_wfopen_s(&file, filePath, L"wb") == 0);
#else
    file = _wfopen(filePath, L"wb");
#endif
    assert(file != NULL);
    return file;
}

static void WriteFileAtPath(const wchar_t *filePath, const char *content)
{
    FILE *file = OpenTestFile(filePath);

    assert(fputs(content, file) >= 0);
    assert(fclose(file) == 0);
}

static void WriteFile(const char *content)
{
    WriteFileAtPath(TEST_FILE_PATH, content);
}

static void TestNormalCsv(void)
{
    ScheduleCollection collection;
    StorageLoadResult result;
    const Schedule *items[4];
    size_t count;

    WriteFile(SCHEDULE_CSV_HEADER "\n"
        "1,2026-08-25,13:00,14:30,Later,note,planned,worklog\n"
        "2,2026-08-25,09:00,10:00,Earlier,first,planned,worklog\n"
        "3,2026-08-26,18:00,19:00,Other day,note,custom,future-source\n");
    result = Storage_LoadSchedules(TEST_FILE_PATH, &collection);

    assert(result.status == STORAGE_LOAD_OK);
    assert(result.loadedCount == 3);
    assert(result.skippedCount == 0);
    count = Schedule_GetForDate(&collection, 2026, 8, 25, items, 4);
    assert(count == 2);
    assert(strcmp(items[0]->id, "2") == 0);
    assert(strcmp(items[1]->id, "1") == 0);
}

static void TestMissingFile(void)
{
    ScheduleCollection collection;
    StorageLoadResult result;

    RemoveTestFile(MISSING_FILE_PATH);
    result = Storage_LoadSchedules(MISSING_FILE_PATH, &collection);
    assert(result.status == STORAGE_LOAD_FILE_NOT_FOUND);
    assert(collection.count == 0);
}

static void TestTimeOrderValidation(void)
{
    ScheduleCollection collection;
    StorageLoadResult result;

    WriteFile(SCHEDULE_CSV_HEADER "\n"
        "1,2026-08-25,09:00,10:00,Normal,note,planned,worklog\n"
        "2,2026-08-25,10:00,10:00,Same time,note,planned,worklog\n"
        "3,2026-08-25,10:00,09:00,Reverse,note,planned,worklog\n"
        "4,2026-08-25,00:00,00:01,First minute,note,planned,worklog\n"
        "5,2026-08-25,23:58,23:59,Last minute,note,planned,worklog\n"
        "6,2026-08-25,11:00,12:00,Valid after invalid,note,planned,worklog\n");
    result = Storage_LoadSchedules(TEST_FILE_PATH, &collection);

    assert(result.status == STORAGE_LOAD_OK);
    assert(result.loadedCount == 4);
    assert(result.skippedCount == 2);
    assert(collection.count == 4);
}

static void TestDuplicateIdsAreSkipped(void)
{
    ScheduleCollection collection;
    StorageLoadResult result;

    WriteFile(SCHEDULE_CSV_HEADER "\n"
        "same,2026-08-25,09:00,10:00,First,note,planned,worklog\n"
        "same,2026-08-26,11:00,12:00,Duplicate,note,planned,worklog\n"
        "different,2026-08-25,09:00,10:00,Same time,note,planned,worklog\n"
        "after,2026-08-27,13:00,14:00,After duplicate,note,planned,worklog\n");
    result = Storage_LoadSchedules(TEST_FILE_PATH, &collection);

    assert(result.status == STORAGE_LOAD_OK);
    assert(result.loadedCount == 3);
    assert(result.skippedCount == 1);
    assert(collection.count == 3);
    assert(strcmp(collection.items[0].id, "same") == 0);
    assert(collection.items[0].day == 25);
    assert(strcmp(collection.items[1].id, "different") == 0);
    assert(strcmp(collection.items[2].id, "after") == 0);
}

static void TestUnicodeFilePaths(void)
{
    ScheduleCollection collection;
    StorageLoadResult result;

    WriteFileAtPath(UNICODE_FILE_PATH, SCHEDULE_CSV_HEADER "\n"
        "unicode,2026-08-25,09:00,10:00,Unicode path,note,planned,worklog\n");
    result = Storage_LoadSchedules(UNICODE_FILE_PATH, &collection);
    assert(result.status == STORAGE_LOAD_OK);
    assert(result.loadedCount == 1);
    assert(strcmp(collection.items[0].id, "unicode") == 0);

    RemoveTestFile(MISSING_UNICODE_FILE_PATH);
    result = Storage_LoadSchedules(MISSING_UNICODE_FILE_PATH, &collection);
    assert(result.status == STORAGE_LOAD_FILE_NOT_FOUND);
    assert(collection.count == 0);
}

static void TestEmptyAndHeaderOnly(void)
{
    ScheduleCollection collection;
    StorageLoadResult result;

    WriteFile("");
    result = Storage_LoadSchedules(TEST_FILE_PATH, &collection);
    assert(result.status == STORAGE_LOAD_OK);
    assert(collection.count == 0);

    WriteFile(SCHEDULE_CSV_HEADER "\n");
    result = Storage_LoadSchedules(TEST_FILE_PATH, &collection);
    assert(result.status == STORAGE_LOAD_OK);
    assert(collection.count == 0);
}

static void TestInvalidRowsAreSkipped(void)
{
    ScheduleCollection collection;
    StorageLoadResult result;

    WriteFile(SCHEDULE_CSV_HEADER "\n"
        "bad row\n"
        "1,2026-02-29,09:00,10:00,Bad date,note,planned,worklog\n"
        "2,2026-08-25,25:00,10:00,Bad time,note,planned,worklog\n"
        "3,2026-08-25,09:00,10:00,Valid,note,planned,worklog\n");
    result = Storage_LoadSchedules(TEST_FILE_PATH, &collection);

    assert(result.status == STORAGE_LOAD_OK);
    assert(result.loadedCount == 1);
    assert(result.skippedCount == 3);
    assert(collection.count == 1);
}

static void TestQuotedFields(void)
{
    ScheduleCollection collection;
    StorageLoadResult result;

    WriteFile(SCHEDULE_CSV_HEADER "\n"
        "1,2026-08-25,09:00,10:00,\"Title, with comma\","
        "\"Quoted \"\"note\"\"\",planned,worklog\n");
    result = Storage_LoadSchedules(TEST_FILE_PATH, &collection);

    assert(result.loadedCount == 1);
    assert(result.skippedCount == 0);
    assert(strcmp(collection.items[0].title, "Title, with comma") == 0);
    assert(strcmp(collection.items[0].note, "Quoted \"note\"") == 0);
}

static void TestLongRowIsSkipped(void)
{
    ScheduleCollection collection;
    StorageLoadResult result;
    FILE *file = OpenTestFile(TEST_FILE_PATH);
    int index;

    assert(fputs(SCHEDULE_CSV_HEADER "\n", file) >= 0);
    for (index = 0; index < 2200; ++index) {
        assert(fputc('x', file) != EOF);
    }
    assert(fputs("\n4,2026-08-31,09:00,10:00,Valid after long row,"
        "note,planned,worklog\n", file) >= 0);
    assert(fclose(file) == 0);

    result = Storage_LoadSchedules(TEST_FILE_PATH, &collection);
    assert(result.status == STORAGE_LOAD_OK);
    assert(result.loadedCount == 1);
    assert(result.skippedCount == 1);
    assert(collection.items[0].day == 31);
}

int main(void)
{
    assert(atexit(CleanupTestFiles) == 0);
    CleanupTestFiles();
    TestNormalCsv();
    TestMissingFile();
    TestTimeOrderValidation();
    TestDuplicateIdsAreSkipped();
    TestUnicodeFilePaths();
    TestEmptyAndHeaderOnly();
    TestInvalidRowsAreSkipped();
    TestQuotedFields();
    TestLongRowIsSkipped();
    CleanupTestFiles();

    puts("Storage CSV tests passed.");
    return 0;
}
