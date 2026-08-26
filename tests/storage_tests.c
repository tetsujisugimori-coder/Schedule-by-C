#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "storage.h"

#define TEST_FILE_PATH "storage_test_schedule.csv"
#define MISSING_FILE_PATH "storage_test_missing.csv"

static FILE *OpenTestFile(void)
{
    FILE *file = NULL;

#ifdef _MSC_VER
    assert(fopen_s(&file, TEST_FILE_PATH, "wb") == 0);
#else
    file = fopen(TEST_FILE_PATH, "wb");
#endif
    assert(file != NULL);
    return file;
}

static void WriteFile(const char *content)
{
    FILE *file = OpenTestFile();

    assert(fputs(content, file) >= 0);
    assert(fclose(file) == 0);
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

    remove(MISSING_FILE_PATH);
    result = Storage_LoadSchedules(MISSING_FILE_PATH, &collection);
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
    FILE *file = OpenTestFile();
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
    TestNormalCsv();
    TestMissingFile();
    TestEmptyAndHeaderOnly();
    TestInvalidRowsAreSkipped();
    TestQuotedFields();
    TestLongRowIsSkipped();
    remove(TEST_FILE_PATH);

    puts("Storage CSV tests passed.");
    return 0;
}
