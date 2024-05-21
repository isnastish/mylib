## Overview
This is small C++ container library with custom memory allocation strategy using memory arenas, designed with thread-safety and performance in mind. For more information please refer to the [architecure](./architecture.md) document.

## Building the project
After cloning the repository, run `git submodule update --init --recursive` to download the source code for all submodules that this project depends on. Run `cmake -S . -B build` to configure, and `cmake -build build` to compile the project.