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

find_package(Threads REQUIRED)

add_executable(aquaguard src/main.c)
target_link_libraries(aquaguard PRIVATE aquaguard_lib Threads::Threads)

# Tests
enable_testing()
add_executable(unit_tests tests/test_parser.c src/json.c)
target_include_directories(unit_tests PRIVATE include)
add_test(NAME parser_test COMMAND unit_tests)

# Install (optional)
install(TARGETS aquaguard RUNTIME DESTINATION bin)
