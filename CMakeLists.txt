# specify minimum cmake version
cmake_minimum_required(VERSION 3.23)

# sets C++ standard
set(CMAKE_CXX_STANDARD 20)

project("cpp_container_library")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Build a static container library
add_library(
    containers_lib
    STATIC
    "./src/containers/growing_array.h"
    "./src/containers/linked_list.h"
    "./src/containers/hash_map.h"
    "./src/containers/string.cpp"
    "./src/containers/string.h"
    "./src/containers/set.h"
    "./src/containers/queue.h"
    "./src/containers/stack.h"
    "./src/containers/arena.h"
    "./src/containers/arena.cpp"
)

target_include_directories(
    containers_lib
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
        containers_lib
)

add_subdirectory("./ext/googletest/")

add_executable(
    test_containers
    "./test/test_growing_array.cpp"
    "./test/test_string.cpp"
    "./test/timer.h"
    "./test/timer.cpp"
    "./test/test_arena.cpp"
)

target_link_libraries(
    test_containers
    PUBLIC
        containers_lib
        gtest
        gtest_main
)

include(GoogleTest)

gtest_discover_tests(
    test_containers
)