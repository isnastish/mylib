cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)

project("mylib")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory("./external/googletest/")
add_subdirectory("./external/fmt/")

add_library(
    mylib
    STATIC
    "./src/mylib/growing_array.h"
    "./src/mylib/linked_list.h"
    "./src/mylib/hash_map.h"
    "./src/mylib/string.cpp"
    "./src/mylib/string.h"
    "./src/mylib/set.h"
    "./src/mylib/queue.h"
    "./src/mylib/stack.h"
    "./src/mylib/arena.h"
    "./src/mylib/arena.cpp"
)

target_link_libraries(
    mylib
    PUBLIC
        fmt
)

target_include_directories(
    mylib
    PUBLIC
        "./src"
)

add_executable(
    main
    "./src/main.cpp"
)

target_link_libraries(
    main
    PUBLIC
        mylib
)

add_executable(
    test_mylib
    "./test/test_growing_array.cpp"
    "./test/test_string.cpp"
    "./test/timer.h"
    "./test/timer.cpp"
    "./test/test_arena.cpp"
)

target_link_libraries(
    test_mylib
    PUBLIC
        mylib
        gtest
        fmt
        gtest_main
)

include(GoogleTest)

gtest_discover_tests(
    test_mylib
)