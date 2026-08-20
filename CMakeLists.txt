cmake_minimum_required(VERSION 3.20)

project(ScheduleByC VERSION 0.1 LANGUAGES C)

add_executable(ScheduleByC WIN32
    src/main.c
    src/calendar.c
)

target_include_directories(ScheduleByC PRIVATE src)
target_compile_definitions(ScheduleByC PRIVATE UNICODE _UNICODE)
target_link_libraries(ScheduleByC PRIVATE user32 gdi32)

if(MSVC)
    target_compile_options(ScheduleByC PRIVATE /W4 /utf-8)
endif()

enable_testing()

add_executable(CalendarTests
    tests/calendar_tests.c
    src/calendar.c
)

target_include_directories(CalendarTests PRIVATE src)

if(MSVC)
    target_compile_options(CalendarTests PRIVATE /W4 /utf-8)
endif()

add_test(NAME CalendarTests COMMAND CalendarTests)
