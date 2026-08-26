#include "storage.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calendar.h"

#define CSV_FIELD_COUNT 8
#define MAX_CSV_FIELD_LENGTH MAX_NOTE_LENGTH
#define MAX_CSV_LINE_LENGTH 2048

static FILE *OpenFileForReading(const wchar_t *filePath)
{
    FILE *file = NULL;

#ifdef _MSC_VER
    errno_t openError = _wfopen_s(&file, filePath, L"rb");
    if (openError != 0) {
        errno = (int)openError;
        return NULL;
    }
#else
    file = _wfopen(filePath, L"rb");
#endif
    return file;
}

static void RemoveLineEnding(char *line)
{
    size_t length = strlen(line);

    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[--length] = '\0';
    }
}

static int IsLineComplete(const char *line, FILE *file)
{
    size_t length = strlen(line);

    return length == 0 || line[length - 1] == '\n' || feof(file);
}

static void DiscardLineRemainder(FILE *file)
{
    int character;

    do {
        character = fgetc(file);
    } while (character != '\n' && character != EOF);
}

static int CopyField(char *destination, size_t destinationSize,
    const char *source)
{
    size_t length = strlen(source);

    if (length >= destinationSize) {
        return 0;
    }

    memcpy(destination, source, length + 1);
    return 1;
}

static int ParseCsvLine(const char *line,
    char fields[CSV_FIELD_COUNT][MAX_CSV_FIELD_LENGTH + 1])
{
    size_t position = 0;
    int fieldIndex;

    for (fieldIndex = 0; fieldIndex < CSV_FIELD_COUNT; ++fieldIndex) {
        size_t outputLength = 0;
        int quoted = line[position] == '"';

        if (quoted) {
            position++;
            for (;;) {
                char character = line[position];

                if (character == '\0') {
                    return 0;
                }
                if (character == '"') {
                    if (line[position + 1] == '"') {
                        character = '"';
                        position += 2;
                    } else {
                        position++;
                        break;
                    }
                } else {
                    position++;
                }

                if (outputLength >= MAX_CSV_FIELD_LENGTH) {
                    return 0;
                }
                fields[fieldIndex][outputLength++] = character;
            }

            if (line[position] != ',' && line[position] != '\0') {
                return 0;
            }
        } else {
            while (line[position] != ',' && line[position] != '\0') {
                if (line[position] == '"'
                    || outputLength >= MAX_CSV_FIELD_LENGTH) {
                    return 0;
                }
                fields[fieldIndex][outputLength++] = line[position++];
            }
        }

        fields[fieldIndex][outputLength] = '\0';

        if (fieldIndex < CSV_FIELD_COUNT - 1) {
            if (line[position] != ',') {
                return 0;
            }
            position++;
        } else if (line[position] != '\0') {
            return 0;
        }
    }

    return 1;
}

static int ParseFixedDigits(const char *text, size_t start, size_t count,
    int *value)
{
    size_t index;
    int result = 0;

    for (index = 0; index < count; ++index) {
        char character = text[start + index];

        if (character < '0' || character > '9') {
            return 0;
        }
        result = result * 10 + (character - '0');
    }

    *value = result;
    return 1;
}

static int ParseDate(const char *text, int *year, int *month, int *day)
{
    if (strlen(text) != 10 || text[4] != '-' || text[7] != '-') {
        return 0;
    }

    if (!ParseFixedDigits(text, 0, 4, year)
        || !ParseFixedDigits(text, 5, 2, month)
        || !ParseFixedDigits(text, 8, 2, day)) {
        return 0;
    }

    return Calendar_IsValidDate(*year, *month, *day);
}

static int ParseTimeInMinutes(const char *text, int *minutesFromMidnight)
{
    int hour;
    int minute;

    if (strlen(text) != MAX_TIME_LENGTH || text[2] != ':') {
        return 0;
    }

    if (!ParseFixedDigits(text, 0, 2, &hour)
        || !ParseFixedDigits(text, 3, 2, &minute)
        || hour < 0 || hour > 23
        || minute < 0 || minute > 59) {
        return 0;
    }

    *minutesFromMidnight = hour * 60 + minute;
    return 1;
}

static int ConvertFieldsToSchedule(
    char fields[CSV_FIELD_COUNT][MAX_CSV_FIELD_LENGTH + 1],
    Schedule *schedule)
{
    int startMinutes;
    int endMinutes;

    memset(schedule, 0, sizeof(*schedule));

    if (fields[0][0] == '\0' || fields[4][0] == '\0'
        || fields[6][0] == '\0' || fields[7][0] == '\0'
        || !ParseDate(fields[1], &schedule->year, &schedule->month,
            &schedule->day)
        || !ParseTimeInMinutes(fields[2], &startMinutes)
        || !ParseTimeInMinutes(fields[3], &endMinutes)
        || startMinutes >= endMinutes) {
        return 0;
    }

    return CopyField(schedule->id, sizeof(schedule->id), fields[0])
        && CopyField(schedule->startTime, sizeof(schedule->startTime), fields[2])
        && CopyField(schedule->endTime, sizeof(schedule->endTime), fields[3])
        && CopyField(schedule->title, sizeof(schedule->title), fields[4])
        && CopyField(schedule->note, sizeof(schedule->note), fields[5])
        && CopyField(schedule->status, sizeof(schedule->status), fields[6])
        && CopyField(schedule->source, sizeof(schedule->source), fields[7]);
}

StorageLoadResult Storage_LoadSchedules(const wchar_t *filePath,
    ScheduleCollection *collection)
{
    StorageLoadResult result = { STORAGE_LOAD_OK, 0, 0, 0 };
    char line[MAX_CSV_LINE_LENGTH + 2];
    FILE *file;
    int firstContentLine = 1;

    if (collection == NULL || filePath == NULL) {
        result.status = STORAGE_LOAD_READ_ERROR;
        result.errorNumber = EINVAL;
        return result;
    }

    ScheduleCollection_Init(collection);
    errno = 0;
    file = OpenFileForReading(filePath);
    if (file == NULL) {
        result.errorNumber = errno;
        result.status = (errno == ENOENT)
            ? STORAGE_LOAD_FILE_NOT_FOUND : STORAGE_LOAD_READ_ERROR;
        return result;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char fields[CSV_FIELD_COUNT][MAX_CSV_FIELD_LENGTH + 1];
        Schedule schedule;
        char *content = line;

        if (!IsLineComplete(line, file)) {
            DiscardLineRemainder(file);
            result.skippedCount++;
            firstContentLine = 0;
            continue;
        }

        RemoveLineEnding(line);
        if (firstContentLine && strlen(content) >= 3
            && (unsigned char)content[0] == 0xEF
            && (unsigned char)content[1] == 0xBB
            && (unsigned char)content[2] == 0xBF) {
            content += 3;
        }
        if (content[0] == '\0') {
            continue;
        }

        if (firstContentLine) {
            firstContentLine = 0;
            if (strcmp(content, SCHEDULE_CSV_HEADER) == 0) {
                continue;
            }
            result.status = STORAGE_LOAD_INVALID_FORMAT;
            break;
        }

        if (!ParseCsvLine(content, fields)
            || !ConvertFieldsToSchedule(fields, &schedule)
            || !ScheduleCollection_Add(collection, &schedule)) {
            result.skippedCount++;
            continue;
        }

        result.loadedCount++;
    }

    if (ferror(file)) {
        result.status = STORAGE_LOAD_READ_ERROR;
        result.errorNumber = errno != 0 ? errno : EIO;
    }
    fclose(file);
    return result;
}
