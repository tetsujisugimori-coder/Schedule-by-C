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

## 2026-08-26: PR #5 review fixes

- Separated the tracked sample from runtime data by renaming `schedule.csv` to `schedule.example.csv` and adding the real `schedule.csv` to `.gitignore`.
- Changed the CMake post-build step to copy only `schedule.example.csv` under its sample name. Builds no longer create, update, or overwrite runtime `schedule.csv` data.
- Converted validated `HH:MM` values to minutes from midnight and now reject schedules where `start_time >= end_time`. Overnight schedules remain unsupported.
- Enforced non-empty, unique schedule IDs in `ScheduleCollection_Add`. The first valid occurrence is retained, while later duplicate rows are skipped and counted.
- Changed schedule file paths to wide strings and replaced ANSI path APIs with `GetEnvironmentVariableW`, `GetModuleFileNameW`, and wide-character file opening. CSV contents remain UTF-8.
- Added a minimum client size through `WM_GETMINMAXINFO` and separated vertical layout calculation. The minimum size preserves the header, weekday row, six calendar rows, and schedule footer without overlap or footer overflow.
- Updated `README.md` with the sample/runtime file distinction, non-overwriting build behavior, time-order rule, duplicate-ID behavior, Unicode path support, minimum window size, and current limitations.

## Verification for PR #5 review fixes

- Built the Debug configuration with MSVC `/W4`: 0 warnings and 0 errors.
- Ran CTest: `CalendarTests`, `ScheduleTests`, and `StorageTests` all passed (3/3).
- Built the application and all tests with MSYS2 UCRT64 GCC using `-Wall -Wextra -Wpedantic -Werror`; all builds and tests passed.
- Added tests for equal/reversed times, `00:00` to `00:01`, `23:58` to `23:59`, valid rows after invalid rows, duplicate IDs, same-time schedules with different IDs, and missing/valid paths containing Japanese text, spaces, and an emoji.
- Compared the SHA-256 hash of an existing `build/Debug/schedule.csv` before and after reconfiguration and rebuild; it remained `0434C0469574458DB796A05B7DE2CBDDA974D2E776F358F297B9A43DFCEFF1FA`. The sample was copied separately as `schedule.example.csv`.
- Performed a GUI smoke test with a path containing Japanese text, a space, and an emoji. Confirmed schedule markers, Japanese titles and notes, start-time ordering, minimum-size layout, date clicking, expansion and repaint, previous/next month navigation, and normal startup with `schedule.csv` missing.
- Remaining constraints: no overnight or multiline CSV schedules, no live reload, fixed collection and field limits, no paths exceeding the configured path buffer, and no schedule creation/editing/deletion or external API communication.

## 2026-08-26: Build-specific README examples

- Separated the MSVC/Visual Studio and MinGW/MSYS2 UCRT64 GCC command examples in `README.md` so each build, sample-data copy, application launch, and CTest command forms one consistent workflow.
- Documented the build-specific sample-copy destinations: `build\Debug\schedule.csv` for MSVC and `build\schedule.csv` for MinGW/GCC.
- Clarified that copying `schedule.example.csv` is unnecessary when `SCHEDULE_BY_C_DATA_FILE` points to a shared schedule file.
- Documented that generator choice changes output paths, that commands for different generators should not be mixed, and that separate directories such as `build-msvc` and `build-gcc` are safer when using multiple generators.

## Verification for build-specific README examples

- Ran `cmake --build build --config Debug` with MSVC successfully.
- Ran `ctest --test-dir build -C Debug --output-on-failure`; `CalendarTests`, `ScheduleTests`, and `StorageTests` all passed (3/3).
- Confirmed the documented MSVC files exist at `build\Debug\ScheduleByC.exe`, `build\Debug\schedule.example.csv`, and `build\Debug\schedule.csv`.
- The MSYS2 UCRT64 GCC compiler remained available, so the application and all three test executables were rebuilt directly with `-Wall -Wextra -Wpedantic -Werror`; all three tests passed. GCC CTest was not run because no MinGW Make or Ninja build tool was available for a CMake generator in the current environment.
- This was a documentation-only change. No C source code or CMake behavior was changed.

## 2026-08-26: Shared local schedule CSV and live reload

- Confirmed this repository is Schedule by C, the read-only consumer of the shared schedule CSV. No files in the separate worklog application repository were changed.
- Replaced the executable-directory and current-directory lookup with `src/schedule_path.c` and `src/schedule_path.h`. The resolver now prefers a non-empty `SCHEDULE_CSV_PATH`, otherwise uses the Windows Documents known folder plus `ScheduleData\schedule.csv`.
- Kept path handling Unicode-native and made environment-path overflow, invalid arguments, and Documents-folder lookup failure explicit errors. Added the required `shell32` and `ole32` link dependencies in CMake.
- Added `src/schedule_data.c` and `src/schedule_data.h` to re-resolve the path for every load and load into a temporary collection. A successful read atomically replaces the displayed schedules; missing files, invalid headers, read errors, and path errors preserve the last successfully displayed collection.
- Added `R` as the visible GUI reload shortcut. The footer now shows reload success or failure, invalid-row counts, a specific failure reason, whether prior data was retained, and the attempted full path.
- Kept Schedule by C read-only. It does not create `ScheduleData`, create or update `schedule.csv`, delete old build CSV files, or copy runtime CSV data between applications.
- Tightened top-level CSV validation so a non-empty file must begin with the existing `id,date,start_time,end_time,title,note,status,source` header. Existing per-row validation and partial loading of invalid data rows remain unchanged.
- Updated the README to document the shared-file contract, default Documents path, `SCHEDULE_CSV_PATH`, the PowerShell example, reopening a terminal after persistent environment changes, and the `R` reload operation.

## Verification for shared local schedule CSV and live reload

- Before changes, configured and built the existing MSVC Debug build and ran CTest: all existing tests passed (3/3); there were no pre-existing failures.
- After changes, built the MSVC Debug configuration with `/W4`: all application and test targets built successfully.
- Ran CTest: `CalendarTests`, `ScheduleTests`, `StorageTests`, `SchedulePathTests`, and `ScheduleDataTests` all passed (5/5).
- `SchedulePathTests` verifies `SCHEDULE_CSV_PATH` priority, insufficient-buffer failure, and the Windows Documents default without writing to Documents.
- `ScheduleDataTests` uses a unique Windows temporary directory and `SCHEDULE_CSV_PATH`. It verifies missing-file startup behavior, replacement after a CSV record is added, path re-resolution, invalid-header retention, and read-error retention.
- Remaining constraints: schedules that cross midnight and multiline CSV fields are unsupported; in-memory collection and field sizes remain fixed; Schedule by C intentionally provides no schedule creation, editing, deletion, directory creation, or external calendar API operations.
