---
title: "Architecture"
weight: 2
---

### Memory Arena 
The code for memory arena and all its auxiliary functionality can be found in `./src/containers/arena.h`
and its corresponding implementation file. For further investigation about memory I highly recommend watching this playlist [General Purpose Allocation](https://www.youtube.com/watch?v=MvDUe2evkHg&list=PLEMXAbCVnmY6Azbmzj3BiC3QRYHE9QoG7&ab_channel=MollyRocket) from Casey Muratori, a developer with more than 30 years of programming experience.

The way I implemented an arena is pretty common. In arena's ctor, we make a call to an operating system to allocate memory of a specified size. On Windows I'm using [VirtualAlloc](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc). That gives us back a giant pull of memory which we can use however we want, 
for example split it up into sub-chunks and allocate memory in pages.
In my case, I decided to go with a separate helper class `MemoryChunk` which serves as a proxy between a client who requested the memory chunk, a growing array for example, and a memory arena. All memory chunks are stored inside the memory arena itself, 
so we don't need any additional allocations to maintain them. The arena itself keeps the list of pointers to chunks and provides them on demand. In order to access the memory, a client needs two things, an actual arena to get/release chunks, and a pointer 
to a chunk to insert the data.

The API is constrained to two main functions `get_memory_chunk`, which takes the size and a pointer to an old chunk as its arguments, and returns a new chunk which satisfies the desired size, and `release_memory_chunk`, which puts the chunk into a state so it can be reused by others. When a new memory chunk is created, it is pushed into a [std::list](https://en.cppreference.com/w/cpp/container/list) together with a corresponding state `ChunkState::InUse`. When a chunk is released, it's state is automatically set to `ChunkState::Free`, which signifies 
that nobody uses this chunk any longer and it can be grabbed on subsequent calls to the earlier mentioned get_memory_chunk procedure.
While requesting for a new memory chunk, we iterate throgh all the available chunks with a state `ChunkState::Free`, and if we find the one with a suitable size we return it rather than creating a new one.

Regardless of the size specified when calling get_memory_chunk, the alignmend takes place and the initial size will be rounded to the  
`mylib::MemoryArena::PAGE_SIZE`, which is currently 1Kib or 1024 bytes. Thus, if asking for 256bytes, a chunk of size 1024 will be created and returned.

### Thread-safety
The reason why we maintain a separate abstraction in a form of a chunk is so we can push objects to memory without locking a mutex. This is a way to achieve lock-free programming. So we are fully in control of our chunk that we've allocated. If we run out of space in a chunk that we (some data structure) owns, we should request a new chunk from arena and return the current one back using
`getChunk()` API call.