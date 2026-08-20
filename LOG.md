# Development Log

## 2026-08-20: v0.1 initial implementation

- Added a C and Win32 API monthly calendar application named `Schedule by C`.
- Implemented calendar calculations separately in `src/calendar.c`: leap years, days in month, day of week, previous month, and next month.
- Added GDI drawing for a 7-column by 6-row calendar grid, weekday headings, month navigation, today highlighting, selected-day highlighting, and a selected-date footer.
- Stored each visible date's drawing rectangle in `CalendarCell` and used the same rectangle with `PtInRect` for click selection.
- Recalculate the calendar layout and hit-test rectangles on repaint after `WM_SIZE`.
- Added `CalendarTests` for 2026-08, 2026-02, 2024-02, and year-boundary month changes.

## Verification

- Built `ScheduleByC.exe` and `CalendarTests.exe` successfully with MSYS2 UCRT64 `gcc`.
- Ran `CalendarTests.exe`: passed.
- Started `ScheduleByC.exe` and confirmed a running main window titled `Schedule by C`.
- Automated visual interaction verification (calendar appearance, click selection, and resize) could not be captured because the local Windows capture helper did not find a screenshot target. These items need one manual GUI pass in the local desktop session.
