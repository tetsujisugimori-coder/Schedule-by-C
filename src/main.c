#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

#include "calendar.h"

#define WINDOW_CLASS_NAME L"ScheduleByCWindow"
#define CALENDAR_COLUMNS 7
#define CALENDAR_ROWS 6
#define MAX_CALENDAR_CELLS (CALENDAR_COLUMNS * CALENDAR_ROWS)

typedef struct {
    int year;
    int month;
    int day;
} CalendarDate;

typedef struct {
    int day;
    RECT rect;
} CalendarCell;

typedef struct {
    int displayedYear;
    int displayedMonth;
    CalendarDate today;
    CalendarDate selectedDate;
    CalendarCell cells[MAX_CALENDAR_CELLS];
    int cellCount;
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
    int firstWeekday;
    int daysInMonth;
    int weekday;
    int day;
    WCHAR text[64];
    HFONT titleFont;
    HFONT oldFont;
    HBRUSH whiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    HBRUSH footerBrush = CreateSolidBrush(RGB(245, 247, 250));
    HBRUSH selectedBrush = CreateSolidBrush(RGB(36, 100, 180));
    HBRUSH todayBrush = CreateSolidBrush(RGB(255, 242, 204));
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(185, 190, 198));
    HPEN todayPen = CreatePen(PS_SOLID, 2, RGB(220, 130, 30));
    HPEN oldPen;

    FillRect(hdc, clientRect, whiteBrush);

    if (gridRight <= gridLeft) {
        gridRight = gridLeft + 7;
    }
    if (footerTop <= gridTop + CALENDAR_ROWS) {
        footerTop = gridTop + CALENDAR_ROWS;
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
    SetBkMode(hdc, TRANSPARENT);
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

    firstWeekday = Calendar_DayOfWeek(g_app.displayedYear, g_app.displayedMonth, 1);
    daysInMonth = Calendar_DaysInMonth(g_app.displayedYear, g_app.displayedMonth);
    g_app.cellCount = 0;
    oldPen = (HPEN)SelectObject(hdc, gridPen);

    for (day = 1; day <= daysInMonth; ++day) {
        int index = firstWeekday + day - 1;
        int row = index / CALENDAR_COLUMNS;
        int column = index % CALENDAR_COLUMNS;
        RECT cellRect;
        RECT dayTextRect;
        int isSelected;
        int isToday;

        cellRect.left = gridLeft + column * columnWidth;
        cellRect.right = (column == CALENDAR_COLUMNS - 1) ? gridRight
            : gridLeft + (column + 1) * columnWidth;
        cellRect.top = gridTop + row * rowHeight;
        cellRect.bottom = (row == CALENDAR_ROWS - 1) ? gridBottom
            : gridTop + (row + 1) * rowHeight;

        isSelected = IsSameDate(g_app.selectedDate, g_app.displayedYear,
            g_app.displayedMonth, day);
        isToday = IsSameDate(g_app.today, g_app.displayedYear,
            g_app.displayedMonth, day);
        FillRect(hdc, &cellRect, isSelected ? selectedBrush
            : (isToday ? todayBrush : whiteBrush));
        Rectangle(hdc, cellRect.left, cellRect.top, cellRect.right, cellRect.bottom);

        if (isToday) {
            SelectObject(hdc, todayPen);
            MoveToEx(hdc, cellRect.left + 1, cellRect.top + 1, NULL);
            LineTo(hdc, cellRect.right - 1, cellRect.top + 1);
            LineTo(hdc, cellRect.right - 1, cellRect.bottom - 1);
            LineTo(hdc, cellRect.left + 1, cellRect.bottom - 1);
            LineTo(hdc, cellRect.left + 1, cellRect.top + 1);
            SelectObject(hdc, gridPen);
        }

        dayTextRect = cellRect;
        dayTextRect.left += 7;
        dayTextRect.top += 6;
        wsprintfW(text, L"%d", day);
        SetTextColor(hdc, isSelected ? RGB(255, 255, 255)
            : (column == 0 ? RGB(190, 50, 50)
            : (column == 6 ? RGB(45, 95, 180) : RGB(35, 39, 47))));
        DrawTextW(hdc, text, -1, &dayTextRect, DT_LEFT | DT_TOP | DT_SINGLELINE);

        g_app.cells[g_app.cellCount].day = day;
        g_app.cells[g_app.cellCount].rect = cellRect;
        ++g_app.cellCount;
    }

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldFont);

    {
        RECT footerRect = { gridLeft, footerTop, gridRight, footerTop + footerHeight };
        FillRect(hdc, &footerRect, footerBrush);
        FrameRect(hdc, &footerRect, (HBRUSH)GetStockObject(GRAY_BRUSH));
        wsprintfW(text, L"選択日: %d年%d月%d日", g_app.selectedDate.year,
            g_app.selectedDate.month, g_app.selectedDate.day);
        InflateRect(&footerRect, -10, -4);
        SetTextColor(hdc, RGB(35, 39, 47));
        DrawTextW(hdc, text, -1, &footerRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    DeleteObject(gridPen);
    DeleteObject(todayPen);
    DeleteObject(footerBrush);
    DeleteObject(selectedBrush);
    DeleteObject(todayBrush);
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
        /* The next WM_PAINT recalculates both drawing rectangles and hit areas. */
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

            for (cellIndex = 0; cellIndex < g_app.cellCount; ++cellIndex) {
                if (PtInRect(&g_app.cells[cellIndex].rect, point)) {
                    g_app.selectedDate.year = g_app.displayedYear;
                    g_app.selectedDate.month = g_app.displayedMonth;
                    g_app.selectedDate.day = g_app.cells[cellIndex].day;
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
