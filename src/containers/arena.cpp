#include "arena.h"

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# undef max
# undef min
#endif

namespace ml
{
MemoryArena::MemoryArena(std::uint64_t size) {
    assert(size != 0);
    if ((size % PAGE_SIZE) != 0)
        size += PAGE_SIZE - (size % PAGE_SIZE); 
    uint8_t *mem = static_cast<uint8_t *>(MemoryArena::alloc_memory(size));
    m_ptr = mem;
    m_cap = size;
    m_pos = 0;
}

MemoryArena::~MemoryArena() {
    MemoryArena::release_memory(m_ptr);
}

MemoryChunk* MemoryArena::get_memory_chunk(MemoryChunk *old_chunk, uint64_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // TODO: While searching for a chunk of appropriate size, we see that a chunk has Free state, 
    // but not enough space, we can check the neighbouring chunk as well, and if it has Free state
    // and sum of their sizes satisfies our needs, we can merge those two chunks together and 
    // return a single, bigger chunk. This process can continue, so we can merge more than one chunk'
    // which are located in a row.
    // We would have to write a recursive (probably) function which looks ahead for chunks with a status Free
    // Or maybe merge chunks only when could find a chunk of an appropriate size?

    // TODO: If right after the chunk comes m_pos, that means that we only have one chunk in a whole arena. 
    // And the chunk itself doesn't have enough memory, we have to extend the current chunk, 
    // rather than creating a new one.
    // TODO: Correctly compute the requested size, because currently it's all busted.
    uint64_t chunk_header_size = sizeof(MemoryChunk);
    uint64_t new_size = (old_chunk == nullptr) ? (size + chunk_header_size) : (old_chunk->m_pos + chunk_header_size + size);
    uint64_t page_count = static_cast<uint64_t>((new_size / static_cast<float>(PAGE_SIZE)) + 1);
    uint64_t total_size = page_count * PAGE_SIZE;
    if ((old_chunk != nullptr) && (m_pos == (old_chunk->m_size + sizeof(MemoryChunk)))) {
        // Extend this chunk instead of creating a new one, so we don't waste a lot of memory on
        // storing MemoryChunk data structures.
        assert((total_size + sizeof(MemoryChunk)) <= m_cap);
        m_pos += (total_size - old_chunk->m_size);
        old_chunk->m_size = total_size;
        return old_chunk;
    }

    // Merge consecutive chunks together if possible.
    // Important to note that chunks has to be consecutive.
    // 1. Find the current memory chunk.
#if 0
    auto old_chunk_itr = std::find(m_chunks.begin(), m_chunks.end(), [&old_chunk](const ChunkPair& pair) {
        if (pair.second == ChunkState::InUse && (old_chunk == pair.first)) {
            return true;
        }
    });
    if (old_chunk_itr != m_chunks.end()) {
        // Explore the consecutive chunks
        uint64_t compound_size = old_chunk->m_size;
        for(auto itr = old_chunk_itr; itr != m_chunks.end(); itr++)  {
            if (itr->second != ChunkState::Free) {
                break;
            }
            // If we start merging chunks, memory won't be in pages of size 1kib anymore.
            // Or we can fit the header inside chunks size.
            // thus, instead of allocating 1024 + sizeof(MemoryCunk), we make the chunk be a part of allocated memory, 
            // and then the size of the chunks would be 1024 - sizeof(MemoryChunk) which is 1000 in our case.
            compound_size += itr->first->m_size + sizeof(MemoryChunk);
        }  
    }
#endif

    auto itr = std::find_if(m_chunks.begin(), m_chunks.end(),
    [total_size](std::pair<MemoryChunk*, ChunkState>& chunk) -> bool {
        if ((chunk.second == ChunkState::Free) && (chunk.first->m_size >= total_size))
            return true;
        return false;
    });
    if (itr != m_chunks.end()) {
        MemoryChunk *chunk = itr->first;
        if (old_chunk != nullptr) {
            copy_chunk(chunk, old_chunk);
        }
        return chunk;
    }

    // NOTE: If this fails, we have to grow the arena, which would invalidate all the pointers.
    // Thus we cannot do the reallocation at this point in time.
    // We cannot grow the arena that easily because all the MemoryChunk pointers will become stale. 
    // We would have to iterate over all the chunks and update pointers.
    assert(total_size <= (m_cap - m_pos));
    MemoryChunk *chunk_ptr = reinterpret_cast<MemoryChunk *>(m_ptr + m_pos);
    // NOTE: Memory chunk is a part of a PAGE_SIZE*page_count
    uint8_t *start_ptr = (m_ptr + m_pos + sizeof(MemoryChunk));
    *chunk_ptr = MemoryChunk(start_ptr, total_size);

    m_pos += total_size;

    m_chunks.push_back({chunk_ptr, ChunkState::InUse});

    if (old_chunk != nullptr) {
        copy_chunk(chunk_ptr, old_chunk);
    }

    return chunk_ptr;
}

void MemoryArena::release_memory_chunk(MemoryChunk *chunk) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (chunk != nullptr) {
        ChunkPair* pair = get_chunk_pair(chunk);
        assert(pair != nullptr);
        pair->second = ChunkState::Free;
        pair->first->m_pos = 0;
    }
}

void MemoryArena::copy_chunk(MemoryChunk *new_chunk, MemoryChunk *old_chunk) {
    ChunkPair* pair = get_chunk_pair(old_chunk);
    assert(pair != nullptr);
    std::memcpy(new_chunk->m_start, old_chunk->m_start, old_chunk->m_pos);
    pair->second = ChunkState::Free;
    pair->first->m_pos = 0;
}

MemoryArena::ChunkPair* MemoryArena::get_chunk_pair(MemoryChunk *chunk) {
    auto chunk_itr = std::find_if(m_chunks.begin(), m_chunks.end(), 
    [chunk](const std::pair<MemoryChunk*, ChunkState>& pair) -> bool {
        if ((pair.first->m_start == chunk->m_start)
            && (pair.second == ChunkState::InUse))
            return true;
        return false;
    });
    return (chunk_itr == m_chunks.end()) ? nullptr : &*chunk_itr; 
}

uint64_t MemoryArena::empty_chunks_count() const { 
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t count = 0;
    std::for_each(m_chunks.begin(), m_chunks.end(), [&count](const ChunkPair& pair) { 
        if (pair.second == ChunkState::Free) 
            count++;
    });
    return count;
}

void MemoryArena::realloc(uint64_t size) {
    uint64_t rem = m_cap - m_pos;
    if (size > rem) {
        uint64_t new_cap = std::max(size + (m_cap - rem), m_cap << 2);
        uint8_t *mem = new uint8_t[new_cap];
        std::memset(mem, 0, new_cap);
        std::memcpy(mem, m_ptr, m_pos);
        uint8_t *tmp = m_ptr;
        m_ptr = mem;
        m_cap = new_cap;
        delete[]tmp; // TODO: Realloc should use alloc_memory as well once it's used.
    }
}

void *MemoryArena::alloc_memory(uint64_t size) {
#ifdef _WIN32
    void *memory = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#else
    void *memory = malloc(size);
    std::memset(memory, 0, size);
#endif
    assert(memory);
    return memory;
}

void MemoryArena::release_memory(void *memory) {
#ifdef _WIN32
    VirtualFree(memory, 0, MEM_RELEASE);
#else
    free(memory);
#endif
}

} // namespace ml