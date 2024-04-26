# specify minimum cmake version
cmake_minimum_required(VERSION 3.23)

# sets C++ standard
set(CMAKE_CXX_STANDARD 20)

# set project name
project("growing_array")

# the output directory for execetuble
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# add_subdirectory("./ext/googletest")

# TODO: Cover interface libraries
# SHARED - dll
# STATIC - lib
# INTERFACE - header only
# MODULE - manually loaded LoadLibrary()
# add_library(
#     string_library # library name
#     STATIC
#     # "./simple_string/main.cpp" 
#     "./simple_string/string.cpp" 
#     "./simple_string/string.h"
# )

# TODO: Read about target_include_directories
# target_include_directories(
#     string_library
#     PUBLIC # all targets that depend on my library will have this include directory
#     # PRIVATE
#     "./simple_string"
# )

# target_compile_features(string_library PUBLIC cxx_std_20)

add_executable(
    growing_array
    "./src/main.cpp"
    "./src/string.cpp"
    "./src/string.h"
    "./src/growing_array.h"
    "./src/hash_map.h"
)

# target_link_libraries(
#     test_exe
#     # string_library
#     gtest # 
# )
