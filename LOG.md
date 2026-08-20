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

## 2026-08-20: 42-cell grid and selection rendering fix

- Changed the monthly view to lay out and draw all 42 cells of the 7-column by 6-row grid before assigning dates.
- Added `Calendar_AssignMonthDays` in `src/calendar.c` so date placement is separate from GDI drawing.
- Extended `CalendarCell` with `hasDay` and kept every cell's `RECT`; hit testing now ignores blank cells.
- Reordered date rendering to fill the cell, redraw its border, and draw its number last. This prevents a selected cell's white text from being hidden by a later fill operation.
- Selected dates use a blue background with white text. Today uses a light background with an orange inner border, and today plus selected keeps the orange inner border over the selected background.

## Verification for the grid fix

- Built the GUI and test executable with MSYS2 UCRT64 `gcc`.
- Ran `CalendarTests-grid.exe`: passed. The test covers Sunday and Saturday month starts, 28/29/30/31-day months, year-boundary navigation, and every one of the 42 assignment slots.
- Started `ScheduleByC-grid.exe`; the process remained active until it was closed for the final rebuild.
- Automated GUI interaction could not run because the local Windows automation native pipe was unavailable. The following manual checks remain: full 42-cell visibility, blank-cell clicks, selected-day text visibility, today-plus-selected visibility, month navigation, and resize hit testing.
