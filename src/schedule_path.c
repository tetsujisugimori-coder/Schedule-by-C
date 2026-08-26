#include "schedule_path.h"

#include <windows.h>
#include <shlobj.h>
#include <string.h>

#define DEFAULT_RELATIVE_PATH L"ScheduleData\\schedule.csv"

static SchedulePathStatus CopyEnvironmentPath(wchar_t *buffer,
    size_t bufferSize, int *wasSet)
{
    DWORD requiredLength;
    DWORD copiedLength;

    SetLastError(ERROR_SUCCESS);
    requiredLength = GetEnvironmentVariableW(SCHEDULE_CSV_PATH_ENV, NULL, 0);
    if (requiredLength == 0) {
        *wasSet = 0;
        return SCHEDULE_PATH_OK;
    }

    *wasSet = 1;
    if ((size_t)requiredLength > bufferSize || bufferSize > MAXDWORD) {
        return SCHEDULE_PATH_BUFFER_TOO_SMALL;
    }

    copiedLength = GetEnvironmentVariableW(SCHEDULE_CSV_PATH_ENV, buffer,
        (DWORD)bufferSize);
    if (copiedLength == 0 || (size_t)copiedLength >= bufferSize) {
        buffer[0] = L'\0';
        return SCHEDULE_PATH_BUFFER_TOO_SMALL;
    }

    return SCHEDULE_PATH_OK;
}

SchedulePathStatus SchedulePath_Resolve(wchar_t *buffer, size_t bufferSize)
{
    PWSTR documentsPath = NULL;
    HRESULT result;
    size_t documentsLength;
    size_t relativeLength = wcslen(DEFAULT_RELATIVE_PATH);
    size_t requiredLength;
    int needsSeparator;
    int environmentWasSet;
    SchedulePathStatus environmentStatus;

    if (buffer == NULL || bufferSize == 0) {
        return SCHEDULE_PATH_INVALID_ARGUMENT;
    }
    buffer[0] = L'\0';

    environmentStatus = CopyEnvironmentPath(buffer, bufferSize,
        &environmentWasSet);
    if (environmentStatus != SCHEDULE_PATH_OK || environmentWasSet) {
        return environmentStatus;
    }

    result = SHGetKnownFolderPath(&FOLDERID_Documents, KF_FLAG_DEFAULT, NULL,
        &documentsPath);
    if (FAILED(result) || documentsPath == NULL || documentsPath[0] == L'\0') {
        if (documentsPath != NULL) {
            CoTaskMemFree(documentsPath);
        }
        return SCHEDULE_PATH_DOCUMENTS_UNAVAILABLE;
    }

    documentsLength = wcslen(documentsPath);
    needsSeparator = documentsPath[documentsLength - 1] != L'\\'
        && documentsPath[documentsLength - 1] != L'/';
    requiredLength = documentsLength + (needsSeparator ? 1u : 0u)
        + relativeLength + 1u;
    if (requiredLength > bufferSize) {
        CoTaskMemFree(documentsPath);
        return SCHEDULE_PATH_BUFFER_TOO_SMALL;
    }

    memcpy(buffer, documentsPath, documentsLength * sizeof(wchar_t));
    if (needsSeparator) {
        buffer[documentsLength++] = L'\\';
    }
    memcpy(buffer + documentsLength, DEFAULT_RELATIVE_PATH,
        (relativeLength + 1u) * sizeof(wchar_t));
    CoTaskMemFree(documentsPath);
    return SCHEDULE_PATH_OK;
}

const wchar_t *SchedulePath_StatusMessage(SchedulePathStatus status)
{
    switch (status) {
    case SCHEDULE_PATH_OK:
        return L"成功";
    case SCHEDULE_PATH_INVALID_ARGUMENT:
        return L"予定ファイルのパス出力先が不正です";
    case SCHEDULE_PATH_BUFFER_TOO_SMALL:
        return L"予定ファイルのパスが長すぎます";
    case SCHEDULE_PATH_DOCUMENTS_UNAVAILABLE:
        return L"Windowsのドキュメントフォルダーを取得できません";
    default:
        return L"予定ファイルのパスを取得できません";
    }
}
