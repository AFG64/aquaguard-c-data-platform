cmake_minimum_required(VERSION 3.15)
project(aquaguard C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_definitions(_DEFAULT_SOURCE)

add_library(aquaguard_lib
    src/json.c
    src/sensor.c
    src/http.c
)
target_include_directories(aquaguard_lib PUBLIC include)
if(NOT WIN32)
  target_link_libraries(aquaguard_lib PRIVATE m)
endif()

find_package(Threads REQUIRED)

add_executable(aquaguard src/main.c)
target_link_libraries(aquaguard PRIVATE aquaguard_lib Threads::Threads)

# Tests
enable_testing()
add_executable(unit_tests tests/test_parser.c src/json.c)
target_include_directories(unit_tests PRIVATE include)
if(NOT WIN32)
  target_link_libraries(unit_tests PRIVATE m)
endif()
add_test(NAME parser_test COMMAND unit_tests)

add_executable(alert_mask_tests tests/test_alert_mask.c src/json.c)
target_include_directories(alert_mask_tests PRIVATE include)
if(NOT WIN32)
  target_link_libraries(alert_mask_tests PRIVATE m)
endif()
add_test(NAME alert_mask_test COMMAND alert_mask_tests)

add_executable(optional_fields_tests tests/test_optional_fields.c src/json.c)
target_include_directories(optional_fields_tests PRIVATE include)
if(NOT WIN32)
  target_link_libraries(optional_fields_tests PRIVATE m)
endif()
add_test(NAME optional_fields_test COMMAND optional_fields_tests)

add_executable(required_fields_tests tests/test_required_fields.c src/json.c)
target_include_directories(required_fields_tests PRIVATE include)
if(NOT WIN32)
  target_link_libraries(required_fields_tests PRIVATE m)
endif()
add_test(NAME required_fields_test COMMAND required_fields_tests)

# Install (optional)
install(TARGETS aquaguard RUNTIME DESTINATION bin)
