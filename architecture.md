---
title: "Architecture"
weight: 2
---

## Memory Arena 
Memory arenas are meant to increase the performance by reducing calls to the operating system in order to acquire 
memory. Ideally, you would have to make a call only once, at a startup of your application or a game, usually the allocation size is 1Gib - 4Gib, but it can vary depending on your needs and limitations of the underlying system. That memory block is split into smaller sub-chunks which are served to the application on a request. Memory arena abstraction is responsible for acquiring and releasing those chunks. Arenas can be implemented in a way that allow auxiliary allocations in order to increase the overall size. If you run out of space in one memory block and there is no empty chunks left, a new call to the operating system will be issued to acquire an additional memory. Thus, multiple arenas can be accumulated together into a linked list data structure.

## Current implementation
The current implementation is written with a couple of things in mind, arenas have to be dynamically growable, thread-safe, in order to be used in a multithreaded environment and writing/reading data has to be lock-free. In addition, the implementation shouldn't rely on any external memory allocations outside the space provided by arena itself, but this problem was only solved partically, see [Possible future improvements](./architecture.md#possible-future-improvements) section for more information. Based on that, I came up with three different abstractions.

### Chunk 
The `Chunk` class is the main user-facing abstraction which is directly used to write data into the memory and read/iterate over it. It's the main responsibility of an arena to make sure that, if the client requests a memory chunk of a specific size, it gets it back, regardless whether it requires an additional allocation or simply finding a free chunk. The client only interacts with the memory through a chunk pointer. Each chunk is stored inside the memory allocated by the arena as a part of a large class `ChunkHeader`, which will be discussed thoroughly in the [Chunk header](./architecture.md#chunk-header) sub-section. The chunk has a pointer to the memory, a size which is an empty space that a chunk holds and a position, which keeps track of where to write the next data. Chunks are acquired by calling `getChunk` arena's member function, and released with a corresponding `releaseChunk` function  if a chunk is no longer needed, or a new one has to be requested, presumably due to the insufficient space in the current one. Arenas mark all released chunks as `FREE` and clear its data.

>**NOTE** It is up to each client to release the chunk if they no longer need it. Chunks which won't be released are left in `IN_USE` state and cannot be reused by other clients. A good practice would be to follow RAII model and release the chunk inside client's destructor.

### Memory block
The `Arena::MemBlock` class represents an actual memory block allocated by the operating system. It is hidden from the clients and is an implementation detail of an arena. Each memory block maintains a doubly linked list of chunks, mentioned earlier, and stores them as a part of its space. Nodes in a linked list are pointers to `ChunkHeader` structures which are created manully by directly casting the memory. That way we can avoid additional memory allocations in order to maintain a linked list, but it comes with a downside in a form of a more complex code. When a client requests a chunk, we iterate throgh the linked list and try to find the one in a free state which satisfies the requested size. If no chunks were found, a new instance of ChunkHeader is created, again by casting the memory, and inserted into the linked list by modifying the corresponding `prev` and `next` pointers. Each chunk header has a state,`ChunkState::IN_USE` if used by a client, or `ChunkState::FREE` otherwise. When a new chunk is created, its state is set to `IN_USE`.

Memory allocations and deallocations are done through the calls to [VirtualAlloc](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc) and a corresponding [VirtualFree](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree) on Windows, or [malloc](https://en.cppreference.com/w/c/memory/malloc) and a correspnding [free](https://en.cppreference.com/w/c/memory/free) on other platforms.

A memory block keeps track of how many chunks it holds, their states and provides a functionality for acquiring/releasing them.

### Chunk header
The `MemBlock::ChunkHeader` class is an abstraction around `Chunk` and a doubly linked list at the same time. It contains of a chunk, its state, and prev and next pointers in order to insert newly created chunks. While computing the size which should be allocated for the current chunk, the `sizeof(ChunkHeader)` is taken into a consideration. Let's take a look at an example. The client requests a chunk of a size 1024bytes(1Kib), we already know that each chunk contains a header (ChunkHeader), thus we have to add the size of the header to initially requested one and align it by the `Arena::PAGE_SIZE` which leads us with allocating a block of memory of 2048bytes or 2Kib. In this case 2048-sizeof(ChunkHeader) will be available for the client to use.

### Arena list
A class `Arena` is the main API that clients will be interacting with, in particular `getChunk` and `releaseChunk` procedures. The arena is a doubly linked list of memory blocks, and this is the only place where we allocate an additional memory. Any call to its public method is thread-safe. Thus, if one thread is releasing a chunk, and another one tries to get a chunk, the second will wait for the chunk to be released, put into `FREE` state, and acquire it after.

## Thread-safety
The reason why we maintain a separate abstraction in a form of a chunk is so we can push objects to memory without locking a mutex. This is a way to achieve lock-free programming. So we are fully in control of our chunk that we've allocated. If we run out of space in a chunk that we (some data structure) owns, we should request a new chunk from arena and return the current one back using
`getChunk` API call.

## Possible future improvements
>**TODO** 

## Useful resources
For further learning about memory arenas I highly recommend watching this playlist [General Purpose Allocation](https://www.youtube.com/watch?v=MvDUe2evkHg&list=PLEMXAbCVnmY6Azbmzj3BiC3QRYHE9QoG7&ab_channel=MollyRocket) from Casey Muratori, a C/C++ developer with more than 30 years of programminng experience.