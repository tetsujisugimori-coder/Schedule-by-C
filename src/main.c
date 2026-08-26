#include <windows.h>
#include <windowsx.h>
#include <string.h>

#include "calendar.h"
#include "schedule.h"
#include "storage.h"

#define WINDOW_CLASS_NAME L"ScheduleByCWindow"
#define CALENDAR_COLUMNS 7
#define CALENDAR_ROWS 6
#define SCHEDULE_MARKER L"*"
#define SCHEDULE_FILE_NAME "schedule.csv"
#define SCHEDULE_PATH_ENV "SCHEDULE_BY_C_DATA_FILE"

typedef struct {
    int year;
    int month;
    int day;
} CalendarDate;

typedef struct {
    RECT rect;
    int hasDay;
    int day;
} CalendarCell;

typedef struct {
    int displayedYear;
    int displayedMonth;
    CalendarDate today;
    CalendarDate selectedDate;
    CalendarCell cells[CALENDAR_GRID_CELL_COUNT];
    RECT previousMonthRect;
    RECT nextMonthRect;
    ScheduleCollection schedules;
    StorageLoadResult loadResult;
} AppState;

static AppState g_app;

static int MinInt(int left, int right)
{
    return left < right ? left : right;
}

static int IsSameDate(CalendarDate left, int year, int month, int day)
{
    return left.year == year && left.month == month && left.day == day;
}

static void GetScheduleFilePath(char path[MAX_PATH])
{
    DWORD environmentLength = GetEnvironmentVariableA(SCHEDULE_PATH_ENV,
        path, MAX_PATH);
    DWORD executableLength;
    char *separator;

    if (environmentLength > 0 && environmentLength < MAX_PATH) {
        return;
    }

    executableLength = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (executableLength == 0 || executableLength >= MAX_PATH) {
        lstrcpynA(path, SCHEDULE_FILE_NAME, MAX_PATH);
        return;
    }

    separator = strrchr(path, '\\');
    if (separator == NULL) {
        lstrcpynA(path, SCHEDULE_FILE_NAME, MAX_PATH);
        return;
    }

    separator[1] = '\0';
    if (lstrlenA(path) + lstrlenA(SCHEDULE_FILE_NAME) >= MAX_PATH) {
        lstrcpynA(path, SCHEDULE_FILE_NAME, MAX_PATH);
        return;
    }
    lstrcatA(path, SCHEDULE_FILE_NAME);
}

static void Utf8ToWide(const char *source, WCHAR *destination,
    int destinationCount)
{
    int converted;

    if (destinationCount <= 0) {
        return;
    }

    converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source, -1,
        destination, destinationCount);
    if (converted == 0) {
        lstrcpynW(destination, L"(文字コードエラー)", destinationCount);
    }
}

static void MoveDisplayedMonth(int direction)
{
    if (direction < 0) {
        Calendar_PreviousMonth(&g_app.displayedYear, &g_app.displayedMonth);
    } else {
        Calendar_NextMonth(&g_app.displayedYear, &g_app.displayedMonth);
    }

    /* Keep a visible selection, clamping 31st to the last day when necessary. */
    g_app.selectedDate.year = g_app.displayedYear;
    g_app.selectedDate.month = g_app.displayedMonth;
    g_app.selectedDate.day = MinInt(g_app.selectedDate.day,
        Calendar_DaysInMonth(g_app.displayedYear, g_app.displayedMonth));
}

static void LayoutCalendarCells(int gridLeft, int gridRight, int gridTop,
    int gridBottom, int columnWidth, int rowHeight)
{
    int cellIndex;

    for (cellIndex = 0; cellIndex < CALENDAR_GRID_CELL_COUNT; ++cellIndex) {
        int row = cellIndex / CALENDAR_COLUMNS;
        int column = cellIndex % CALENDAR_COLUMNS;
        CalendarCell *cell = &g_app.cells[cellIndex];

        cell->rect.left = gridLeft + column * columnWidth;
        cell->rect.right = (column == CALENDAR_COLUMNS - 1) ? gridRight
            : gridLeft + (column + 1) * columnWidth;
        cell->rect.top = gridTop + row * rowHeight;
        cell->rect.bottom = (row == CALENDAR_ROWS - 1) ? gridBottom
            : gridTop + (row + 1) * rowHeight;
        cell->hasDay = FALSE;
        cell->day = 0;
    }
}

static void DrawCalendarGrid(HDC hdc, HBRUSH emptyCellBrush, HBRUSH gridBrush)
{
    int cellIndex;

    for (cellIndex = 0; cellIndex < CALENDAR_GRID_CELL_COUNT; ++cellIndex) {
        FillRect(hdc, &g_app.cells[cellIndex].rect, emptyCellBrush);
        FrameRect(hdc, &g_app.cells[cellIndex].rect, gridBrush);
    }
}

static void AssignDaysToCells(void)
{
    int cellDays[CALENDAR_GRID_CELL_COUNT];
    int cellIndex;

    Calendar_AssignMonthDays(g_app.displayedYear, g_app.displayedMonth, cellDays);
    for (cellIndex = 0; cellIndex < CALENDAR_GRID_CELL_COUNT; ++cellIndex) {
        g_app.cells[cellIndex].hasDay = cellDays[cellIndex] != 0;
        g_app.cells[cellIndex].day = cellDays[cellIndex];
    }
}

static void DrawCalendarDays(HDC hdc, HBRUSH emptyCellBrush,
    HBRUSH selectedBrush, HBRUSH todayBrush, HBRUSH gridBrush, HPEN todayPen)
{
    int cellIndex;
    WCHAR text[16];

    for (cellIndex = 0; cellIndex < CALENDAR_GRID_CELL_COUNT; ++cellIndex) {
        CalendarCell *cell = &g_app.cells[cellIndex];
        RECT dayTextRect;
        int column;
        int isSelected;
        int isToday;
        int hasSchedule;

        if (!cell->hasDay) {
            continue;
        }

        column = cellIndex % CALENDAR_COLUMNS;
        isSelected = IsSameDate(g_app.selectedDate, g_app.displayedYear,
            g_app.displayedMonth, cell->day);
        isToday = IsSameDate(g_app.today, g_app.displayedYear,
            g_app.displayedMonth, cell->day);
        hasSchedule = Schedule_HasForDate(&g_app.schedules,
            g_app.displayedYear, g_app.displayedMonth, cell->day);

        /* Fill first, restore the border, then draw the number last. */
        FillRect(hdc, &cell->rect, isSelected ? selectedBrush
            : (isToday ? todayBrush : emptyCellBrush));
        FrameRect(hdc, &cell->rect, gridBrush);

        if (isToday) {
            HPEN oldPen = (HPEN)SelectObject(hdc, todayPen);
            MoveToEx(hdc, cell->rect.left + 2, cell->rect.top + 2, NULL);
            LineTo(hdc, cell->rect.right - 3, cell->rect.top + 2);
            LineTo(hdc, cell->rect.right - 3, cell->rect.bottom - 3);
            LineTo(hdc, cell->rect.left + 2, cell->rect.bottom - 3);
            LineTo(hdc, cell->rect.left + 2, cell->rect.top + 2);
            SelectObject(hdc, oldPen);
        }

        dayTextRect = cell->rect;
        dayTextRect.left += 7;
        dayTextRect.top += 6;
        wsprintfW(text, L"%d", cell->day);
        SetTextColor(hdc, isSelected ? RGB(255, 255, 255)
            : (column == 0 ? RGB(190, 50, 50)
            : (column == 6 ? RGB(45, 95, 180) : RGB(35, 39, 47))));
        DrawTextW(hdc, text, -1, &dayTextRect, DT_LEFT | DT_TOP | DT_SINGLELINE);

        if (hasSchedule) {
            RECT markerRect = cell->rect;
            markerRect.right -= 7;
            markerRect.top += 4;
            SetTextColor(hdc, isSelected ? RGB(255, 255, 255)
                : RGB(215, 105, 25));
            DrawTextW(hdc, SCHEDULE_MARKER, -1, &markerRect,
                DT_RIGHT | DT_TOP | DT_SINGLELINE);
        }
    }
}

static void DrawScheduleFooter(HDC hdc, const RECT *footerRect,
    HBRUSH footerBrush, HBRUSH gridBrush)
{
    const Schedule *matches[MAX_SCHEDULES];
    size_t matchCount;
    size_t index;
    RECT contentRect = *footerRect;
    WCHAR text[MAX_TITLE_LENGTH + 32];
    WCHAR startTime[MAX_TIME_LENGTH + 1];
    WCHAR endTime[MAX_TIME_LENGTH + 1];
    WCHAR title[MAX_TITLE_LENGTH + 1];
    WCHAR note[MAX_NOTE_LENGTH + 1];
    int y;

    FillRect(hdc, footerRect, footerBrush);
    FrameRect(hdc, footerRect, gridBrush);
    InflateRect(&contentRect, -10, -7);

    matchCount = Schedule_GetForDate(&g_app.schedules,
        g_app.selectedDate.year, g_app.selectedDate.month,
        g_app.selectedDate.day, matches, MAX_SCHEDULES);

    wsprintfW(text, L"選択日: %d年%d月%d日    予定: %u件",
        g_app.selectedDate.year, g_app.selectedDate.month,
        g_app.selectedDate.day, (unsigned int)matchCount);
    SetTextColor(hdc, RGB(35, 39, 47));
    DrawTextW(hdc, text, -1, &contentRect, DT_LEFT | DT_TOP | DT_SINGLELINE);

    {
        RECT statusRect = contentRect;
        statusRect.left = contentRect.left + (contentRect.right - contentRect.left) / 2;
        if (g_app.loadResult.status == STORAGE_LOAD_READ_ERROR) {
            lstrcpynW(text, L"予定データの読み込み中にエラーが発生しました",
                (int)(sizeof(text) / sizeof(text[0])));
        } else if (g_app.loadResult.status == STORAGE_LOAD_FILE_NOT_FOUND
            || g_app.schedules.count == 0) {
            lstrcpynW(text, L"予定データなし", (int)(sizeof(text) / sizeof(text[0])));
        } else if (g_app.loadResult.skippedCount > 0) {
            wsprintfW(text, L"読込: %u件 / スキップ: %u行",
                (unsigned int)g_app.loadResult.loadedCount,
                (unsigned int)g_app.loadResult.skippedCount);
        } else {
            wsprintfW(text, L"読込: %u件",
                (unsigned int)g_app.loadResult.loadedCount);
        }
        SetTextColor(hdc, RGB(90, 96, 108));
        DrawTextW(hdc, text, -1, &statusRect,
            DT_RIGHT | DT_TOP | DT_SINGLELINE);
    }

    y = contentRect.top + 27;
    if (matchCount == 0) {
        RECT messageRect = contentRect;
        messageRect.top = y;
        SetTextColor(hdc, RGB(75, 80, 90));
        DrawTextW(hdc, L"予定はありません。", -1, &messageRect,
            DT_LEFT | DT_TOP | DT_SINGLELINE);
        return;
    }

    for (index = 0; index < matchCount; ++index) {
        const Schedule *schedule = matches[index];
        RECT lineRect = contentRect;

        if (y + 22 > contentRect.bottom) {
            break;
        }

        Utf8ToWide(schedule->title, title,
            (int)(sizeof(title) / sizeof(title[0])));
        Utf8ToWide(schedule->startTime, startTime,
            (int)(sizeof(startTime) / sizeof(startTime[0])));
        Utf8ToWide(schedule->endTime, endTime,
            (int)(sizeof(endTime) / sizeof(endTime[0])));
        wsprintfW(text, L"%s - %s  %s", startTime, endTime, title);
        lineRect.top = y;
        SetTextColor(hdc, RGB(35, 39, 47));
        DrawTextW(hdc, text, -1, &lineRect,
            DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        y += 21;

        if (schedule->note[0] != '\0' && y + 18 <= contentRect.bottom) {
            Utf8ToWide(schedule->note, note,
                (int)(sizeof(note) / sizeof(note[0])));
            lineRect.top = y;
            lineRect.left += 22;
            SetTextColor(hdc, RGB(90, 96, 108));
            DrawTextW(hdc, note, -1, &lineRect,
                DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
            y += 20;
        }
        y += 5;
    }

    if (index < matchCount) {
        RECT remainingRect = contentRect;
        remainingRect.top = contentRect.bottom - 20;
        wsprintfW(text, L"ほか %u件（ウィンドウを縦に広げると表示できます）",
            (unsigned int)(matchCount - index));
        SetTextColor(hdc, RGB(90, 96, 108));
        DrawTextW(hdc, text, -1, &remainingRect,
            DT_RIGHT | DT_TOP | DT_SINGLELINE);
    }
}

static void DrawCalendar(HDC hdc, const RECT *clientRect)
{
    static const WCHAR *weekdays[CALENDAR_COLUMNS] = {
        L"日", L"月", L"火", L"水", L"木", L"金", L"土"
    };
    const int margin = 20;
    const int headerTop = 16;
    const int headerHeight = 46;
    const int weekdayTop = 68;
    const int weekdayHeight = 26;
    int clientWidth = clientRect->right - clientRect->left;
    int clientHeight = clientRect->bottom - clientRect->top;
    int footerHeight = clientHeight / 3;
    int gridLeft = margin;
    int gridRight = clientWidth - margin;
    int gridTop = weekdayTop + weekdayHeight;
    int footerTop = clientHeight - margin - footerHeight;
    int gridBottom;
    int columnWidth;
    int rowHeight;
    int weekday;
    WCHAR text[64];
    HFONT titleFont;
    HFONT oldFont;
    HBRUSH emptyCellBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    HBRUSH gridBrush = CreateSolidBrush(RGB(185, 190, 198));
    HBRUSH footerBrush = CreateSolidBrush(RGB(245, 247, 250));
    HBRUSH selectedBrush = CreateSolidBrush(RGB(36, 100, 180));
    HBRUSH todayBrush = CreateSolidBrush(RGB(255, 242, 204));
    HPEN todayPen = CreatePen(PS_SOLID, 2, RGB(220, 130, 30));

    if (footerHeight < 150) {
        footerHeight = 150;
    }
    if (footerHeight > 280) {
        footerHeight = 280;
    }
    footerTop = clientHeight - margin - footerHeight;

    FillRect(hdc, clientRect, emptyCellBrush);
    SetBkMode(hdc, TRANSPARENT);

    if (gridRight <= gridLeft) {
        gridRight = gridLeft + CALENDAR_COLUMNS;
    }
    if (footerTop <= gridTop + CALENDAR_ROWS + 8) {
        footerTop = gridTop + CALENDAR_ROWS + 8;
    }
    gridBottom = footerTop - 8;
    columnWidth = (gridRight - gridLeft) / CALENDAR_COLUMNS;
    rowHeight = (gridBottom - gridTop) / CALENDAR_ROWS;
    if (columnWidth < 1) {
        columnWidth = 1;
    }
    if (rowHeight < 1) {
        rowHeight = 1;
    }

    titleFont = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Yu Gothic UI");
    oldFont = (HFONT)SelectObject(hdc, titleFont);
    SetTextColor(hdc, RGB(35, 39, 47));
    wsprintfW(text, L"%d年 %d月", g_app.displayedYear, g_app.displayedMonth);
    {
        RECT titleRect = { gridLeft + 52, headerTop, gridRight - 52, headerTop + headerHeight };
        DrawTextW(hdc, text, -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    g_app.previousMonthRect.left = gridLeft;
    g_app.previousMonthRect.top = headerTop;
    g_app.previousMonthRect.right = gridLeft + 42;
    g_app.previousMonthRect.bottom = headerTop + headerHeight;
    g_app.nextMonthRect.left = gridRight - 42;
    g_app.nextMonthRect.top = headerTop;
    g_app.nextMonthRect.right = gridRight;
    g_app.nextMonthRect.bottom = headerTop + headerHeight;
    DrawTextW(hdc, L"<", -1, &g_app.previousMonthRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawTextW(hdc, L">", -1, &g_app.nextMonthRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(titleFont);
    oldFont = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

    for (weekday = 0; weekday < CALENDAR_COLUMNS; ++weekday) {
        RECT weekdayRect;
        weekdayRect.left = gridLeft + weekday * columnWidth;
        weekdayRect.right = (weekday == CALENDAR_COLUMNS - 1) ? gridRight
            : gridLeft + (weekday + 1) * columnWidth;
        weekdayRect.top = weekdayTop;
        weekdayRect.bottom = weekdayTop + weekdayHeight;
        SetTextColor(hdc, weekday == 0 ? RGB(190, 50, 50)
            : (weekday == 6 ? RGB(45, 95, 180) : RGB(50, 55, 65)));
        DrawTextW(hdc, weekdays[weekday], -1, &weekdayRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    LayoutCalendarCells(gridLeft, gridRight, gridTop, gridBottom,
        columnWidth, rowHeight);
    DrawCalendarGrid(hdc, emptyCellBrush, gridBrush);
    AssignDaysToCells();
    DrawCalendarDays(hdc, emptyCellBrush, selectedBrush, todayBrush,
        gridBrush, todayPen);

    SelectObject(hdc, oldFont);

    {
        RECT footerRect = { gridLeft, footerTop, gridRight, footerTop + footerHeight };
        DrawScheduleFooter(hdc, &footerRect, footerBrush, gridBrush);
    }

    DeleteObject(todayPen);
    DeleteObject(todayBrush);
    DeleteObject(selectedBrush);
    DeleteObject(footerBrush);
    DeleteObject(gridBrush);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_PAINT:
        {
            PAINTSTRUCT paint;
            RECT clientRect;
            HDC hdc = BeginPaint(hwnd, &paint);
            GetClientRect(hwnd, &clientRect);
            DrawCalendar(hdc, &clientRect);
            EndPaint(hwnd, &paint);
        }
        return 0;

    case WM_SIZE:
        /* A repaint updates all 42 cell rectangles before the next click. */
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_LBUTTONDOWN:
        {
            POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            int cellIndex;

            if (PtInRect(&g_app.previousMonthRect, point)) {
                MoveDisplayedMonth(-1);
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }
            if (PtInRect(&g_app.nextMonthRect, point)) {
                MoveDisplayedMonth(1);
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }

            for (cellIndex = 0; cellIndex < CALENDAR_GRID_CELL_COUNT; ++cellIndex) {
                CalendarCell *cell = &g_app.cells[cellIndex];
                if (cell->hasDay && PtInRect(&cell->rect, point)) {
                    g_app.selectedDate.year = g_app.displayedYear;
                    g_app.selectedDate.month = g_app.displayedMonth;
                    g_app.selectedDate.day = cell->day;
                    InvalidateRect(hwnd, NULL, TRUE);
                    return 0;
                }
            }
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance,
    LPSTR commandLine, int showCommand)
{
    WNDCLASSW windowClass;
    HWND hwnd;
    MSG message;
    SYSTEMTIME now;
    char scheduleFilePath[MAX_PATH];

    (void)previousInstance;
    (void)commandLine;

    GetLocalTime(&now);
    g_app.today.year = now.wYear;
    g_app.today.month = now.wMonth;
    g_app.today.day = now.wDay;
    g_app.selectedDate = g_app.today;
    g_app.displayedYear = now.wYear;
    g_app.displayedMonth = now.wMonth;
    GetScheduleFilePath(scheduleFilePath);
    g_app.loadResult = Storage_LoadSchedules(scheduleFilePath, &g_app.schedules);

    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.hInstance = instance;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&windowClass)) {
        return 0;
    }

    hwnd = CreateWindowExW(0, WINDOW_CLASS_NAME, L"Schedule by C",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 920, 700,
        NULL, NULL, instance, NULL);
    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return (int)message.wParam;
}
