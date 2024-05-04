# Overview
This is small C++ container library with custom memory allocation strategy using memory arenas technique.
Keep in mind that the project is still deeply in development and most of the data structures are not implemented, or not complete. On the contrary, arenas implementation are considered to be complete, I will probably add more functionality
to it as I iterate over the codebase, but the core will stay the same.
    The final goal for this project is to have a stable implementation of different data structures sharing 
a common memory, allocated by the arena. The arena itself has to be thread-safe and self-growable, 
meaning that, once we've run out of space, the same chunk will be reallocated and all the data preserved.
All external pointers to memory chunks has to be updated during the reallocation. All the functionality has to be
tested thoroughly together with benchmarks against the containers from C++ standard library.