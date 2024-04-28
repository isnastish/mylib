# specify minimum cmake version
cmake_minimum_required(VERSION 3.23)

# sets C++ standard
set(CMAKE_CXX_STANDARD 20)

project("growing_array")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(
    growing_array
    "./src/main.cpp"
    "./src/string.cpp"
    "./src/string.h"
    "./src/growing_array.h"
    "./src/hash_map.h"
    "./src/set.h"
    "./src/linked_list.h"
)
