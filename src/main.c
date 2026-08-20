#include <windows.h>
#include <windowsx.h>

#include "calendar.h"

#define WINDOW_CLASS_NAME L"ScheduleByCWindow"
#define CALENDAR_COLUMNS 7
#define CALENDAR_ROWS 6

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

        if (!cell->hasDay) {
            continue;
        }

        column = cellIndex % CALENDAR_COLUMNS;
        isSelected = IsSameDate(g_app.selectedDate, g_app.displayedYear,
            g_app.displayedMonth, cell->day);
        isToday = IsSameDate(g_app.today, g_app.displayedYear,
            g_app.displayedMonth, cell->day);

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
    const int footerHeight = 42;
    int clientWidth = clientRect->right - clientRect->left;
    int clientHeight = clientRect->bottom - clientRect->top;
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
        FillRect(hdc, &footerRect, footerBrush);
        FrameRect(hdc, &footerRect, gridBrush);
        wsprintfW(text, L"選択日: %d年%d月%d日", g_app.selectedDate.year,
            g_app.selectedDate.month, g_app.selectedDate.day);
        InflateRect(&footerRect, -10, -4);
        SetTextColor(hdc, RGB(35, 39, 47));
        DrawTextW(hdc, text, -1, &footerRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
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

    (void)previousInstance;
    (void)commandLine;

    GetLocalTime(&now);
    g_app.today.year = now.wYear;
    g_app.today.month = now.wMonth;
    g_app.today.day = now.wDay;
    g_app.selectedDate = g_app.today;
    g_app.displayedYear = now.wYear;
    g_app.displayedMonth = now.wMonth;

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
