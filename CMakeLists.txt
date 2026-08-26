cmake_minimum_required(VERSION 3.20)

project(ScheduleByC VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(ScheduleCore
    src/calendar.c
    src/schedule.c
    src/schedule_data.c
    src/schedule_path.c
    src/storage.c
)

target_include_directories(ScheduleCore PUBLIC src)
target_link_libraries(ScheduleCore PUBLIC shell32 ole32)

if(MSVC)
    target_compile_options(ScheduleCore PRIVATE /W4 /utf-8)
else()
    target_compile_options(ScheduleCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(ScheduleByC WIN32
    src/main.c
)

target_compile_definitions(ScheduleByC PRIVATE UNICODE _UNICODE)
target_link_libraries(ScheduleByC PRIVATE ScheduleCore user32 gdi32)

add_custom_command(TARGET ScheduleByC POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/schedule.example.csv"
        "$<TARGET_FILE_DIR:ScheduleByC>/schedule.example.csv"
)

if(MSVC)
    target_compile_options(ScheduleByC PRIVATE /W4 /utf-8)
else()
    target_compile_options(ScheduleByC PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(CalendarTests
    tests/calendar_tests.c
)
target_link_libraries(CalendarTests PRIVATE ScheduleCore)

if(MSVC)
    target_compile_options(CalendarTests PRIVATE /W4 /utf-8)
endif()

add_test(NAME CalendarTests COMMAND CalendarTests)

add_executable(ScheduleTests tests/schedule_tests.c)
target_link_libraries(ScheduleTests PRIVATE ScheduleCore)

add_executable(StorageTests tests/storage_tests.c)
target_link_libraries(StorageTests PRIVATE ScheduleCore)

add_executable(SchedulePathTests tests/schedule_path_tests.c)
target_link_libraries(SchedulePathTests PRIVATE ScheduleCore)

add_executable(ScheduleDataTests tests/schedule_data_tests.c)
target_link_libraries(ScheduleDataTests PRIVATE ScheduleCore)

if(MSVC)
    target_compile_options(ScheduleTests PRIVATE /W4 /utf-8)
    target_compile_options(StorageTests PRIVATE /W4 /utf-8)
    target_compile_options(SchedulePathTests PRIVATE /W4 /utf-8)
    target_compile_options(ScheduleDataTests PRIVATE /W4 /utf-8)
else()
    target_compile_options(CalendarTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ScheduleTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(StorageTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(SchedulePathTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ScheduleDataTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ScheduleTests COMMAND ScheduleTests)
add_test(NAME StorageTests COMMAND StorageTests)
add_test(NAME SchedulePathTests COMMAND SchedulePathTests)
add_test(NAME ScheduleDataTests COMMAND ScheduleDataTests)
