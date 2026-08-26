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

## 2026-08-26: External schedule data display

- Expanded Schedule by C from a monthly calendar into a read-only schedule viewer. Schedule creation, editing, and deletion remain outside this application and are intended to be handled by the worklog application.
- Added `Schedule` and `ScheduleCollection` in `src/schedule.c` and `src/schedule.h`, including date-based lookup, schedule-existence checks, and start-time sorting.
- Added `src/storage.c` and `src/storage.h` to load UTF-8 `schedule.csv` data without coupling the calendar module to the storage format.
- Added validation for required fields, fixed-length buffers, `YYYY-MM-DD` dates, and 24-hour `HH:MM` times. Empty files, headers without records, blank lines, invalid rows, oversized rows, and missing files do not terminate the application.
- Implemented one-record-per-line CSV parsing with quoted comma support and doubled-quote escaping. Multiline CSV fields are intentionally unsupported.
- Unknown `status` and `source` values are retained without changing Schedule by C behavior, allowing future data producers and external integrations to add values safely.
- Added a sample `schedule.csv` and copied it beside the executable during CMake builds. The `SCHEDULE_BY_C_DATA_FILE` environment variable can point to a shared schedule file maintained by the worklog application.
- Added an orange `*` marker to calendar dates that contain schedules and expanded the existing footer into a schedule area. The selected date's schedules are displayed in start-time order with their title and note.
- Preserved the existing 42-cell layout, month navigation, date selection, today highlight, and selected-date highlight.
- Added `ScheduleCore` to the CMake build and separated calendar calculation, schedule operations, CSV storage, and Win32 UI coordination.
- Updated `README.md` with the application role, data flow, CSV schema, field limits, quoting rules, build and execution instructions, current scope, and future Memo Nexus/iCalendar/Google Calendar/Outlook integration direction.

## Verification for external schedule data

- Built the Debug configuration with MSVC using warning level `/W4`: 0 warnings and 0 errors.
- Ran CTest: `CalendarTests`, `ScheduleTests`, and `StorageTests` all passed (3/3).
- Built the application and tests with MSYS2 UCRT64 `gcc` using `-Wall -Wextra -Wpedantic -Werror`; all builds and tests passed.
- Tests cover no schedule, one schedule, multiple schedules, start-time sorting, normal CSV, missing file, empty file, header only, invalid date/time and incomplete rows, quoted fields, oversized rows, leap years, month boundaries, and year-boundary navigation.
- Performed a GUI smoke test of the built application. Confirmed schedule markers on August 25 and 26, the selected-date display, UTF-8 Japanese titles and notes, and two August 25 schedules rendered in 09:00 then 13:00 order.
