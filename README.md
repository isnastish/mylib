# Overview
This is small C++ container library with custom memory allocation strategy using memory arenas technique.
Keep in mind that the project is still deeply in development and most of the data structures are not implemented, or not complete. On the contrary, arenas implementation is considered to be complete, I will probably add more functionality
to it as I iterate over the codebase, but the core will stay the same.
    The final goal for this project is to have a stable implementation of different data structures sharing 
a common memory, allocated by the arena. The arena itself has to be thread-safe and self-growable, 
meaning that, once we've run out of space, the same chunk will be reallocated and all the data preserved.
All external pointers to memory chunks has to be updated during the reallocation. All the functionality has to be
tested thoroughly together with benchmarks against the containers from C++ standard library.

# Memory Arena 
The code for memory arena and all its auxiliary functionality can be found in `./src/containers/arena.h`
and its corresponding implementation file. For further investigation about memory I highly recommend watching this playlist [General Purpose Allocation](https://www.youtube.com/watch?v=MvDUe2evkHg&list=PLEMXAbCVnmY6Azbmzj3BiC3QRYHE9QoG7&ab_channel=MollyRocket) from Casey Muratori, a developer with more than 30 years of programming experience.

The way I implemented an arena is pretty common. In arena's ctor, we make a call to an operating system to allocate memory of a specified size. On Windows I'm using [VirtualAlloc](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc). That gives us back a giant pull of memory which we can use however we want, 
for example split it up into sub-chunks and allocate memory in pages.
In my case, I decided to go with a separate helper class `MemoryChunk`. 