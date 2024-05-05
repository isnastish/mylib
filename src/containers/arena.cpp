#include "arena.h"

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# undef max
# undef min
#endif

namespace mylib
{

Chunk::Chunk(uint8_t *ptr, uint64_t size) : m_start(ptr), m_size(size), m_pos(0) {}

Arena::Arena(std::uint64_t size) {
    constexpr uint64_t arena_size = sizeof(Arena); 
    uint64_t new_size = size + arena_size;
    uint64_t rem = new_size % PAGE_SIZE; 
    new_size += (rem == 0 ? 0 : PAGE_SIZE - rem);
    m_ptr = static_cast<uint8_t*>(allocMemory(new_size));
    m_cap = new_size - arena_size;
    m_pos = 0;
}

Arena::~Arena() {
    releaseMemory(m_ptr);
}

Chunk* Arena::getChunk(uint64_t size, Chunk *old_chunk) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // NOTE: Compute a total allocation size taking into account an alignment.
    // If a chunk is nullptr, we include the size of the Chunk into the total allocation size.
    // Thus, if requested size is equal to 1024, which is exactly the size of a single page (PAGE_SIZE), 
    // two pages will be allocated, comprising the size of 2048bytes in total,
    // because Chunk sturuct occupies 24bytes, thus 1024+24 = 1048 aligned by PAGE_SIZE would be 2048, or 2*PAGE_SIZE.
    // +--------+-------------------+------------------------------------------------------------+
    // | chunk  |         size      |                      empty space                           |
    // | header |         size      |                      empty space                           |
    // +--------+-------------------+------------------------------------------------------------+
    //                              ^
    //                              |
    //                             m_pos
    constexpr static uint64_t chunk_header_size = sizeof(ChunkPair);

    uint64_t new_size = size + chunk_header_size;
    uint64_t rem = new_size % PAGE_SIZE;
    uint64_t new_aligned_size = (rem == 0) ? new_size : (new_size + PAGE_SIZE - rem);
    uint64_t page_count = new_aligned_size / PAGE_SIZE;
    uint64_t total_size = page_count * PAGE_SIZE;

    // NOTE: When looking for a new chunk of memory, we don't just grab the first which satisfies our size requirement.
    // What we do instead is to search for the chunk with size closest to what we want.
    // As we iterate through all the chunks, we compare their sizes and reassign new_chunk pointer if more optimal chunk 
    // has been found.
    // The const of this operation is O(N), where N is the number of chunks stored in a chunks list, 
    // plus the time spend on a size comparison.
    // NOTE: Search for a new chunk in the current arena, if wasn't found, jump to the next arena, 
    // otherwise create a new memory arena of the same size that the current one.
    // We would have to iterate through all the chunks to verify whether there is a chunk of an appropriate size.
    ChunkPair* new_chunk = nullptr;
    for (Arena* arena = this; arena != nullptr; arena = arena->m_next) {
        if (!arena->m_empty_chunks_count)
            continue;
        for (ChunkPair* cur_chunk = m_chunks; cur_chunk != nullptr; cur_chunk = cur_chunk->next) {
            if ((cur_chunk->state == ChunkState::FREE) && 
                (cur_chunk->chunk.size() >= (total_size - chunk_header_size))) {
                if (!new_chunk)
                    new_chunk = cur_chunk;
                else {
                    if (cur_chunk->chunk.size() < new_chunk->chunk.size())
                        new_chunk = cur_chunk;
                }
            }
        }
    }

    if (new_chunk != nullptr) {
        new_chunk->state = ChunkState::IN_USE;
        m_empty_chunks_count -= 1;
    }
    else {
        Arena* arena = this;
        for (; arena != nullptr; arena = arena->m_next) {
            if (total_size < arena->remaining())
                break;
        }

        // NOTE: Create a new memory arena
        if (arena == nullptr) {
            auto* arena = reinterpret_cast<Arena*>(m_ptr + m_cap);
            *arena = std::move(Arena(m_cap));
        }

        // NOTE: Create a new chunk inside the current memory arena.
        ChunkPair* pair = reinterpret_cast<ChunkPair*>(arena->m_ptr + m_pos);
        pair->chunk = std::move(Chunk(m_ptr + sizeof(ChunkPair), total_size));
        pair->state = ChunkState::IN_USE;
        pair->next = nullptr;
        pair->prev = m_chunks->prev; 

        // NOTE: A chunk of a suitable size hasn't been found, 
        // so allocate a new one out of empty space.
        // assert(total_size <= remaining());
        // ChunkPair *chunk_pair = reinterpret_cast<ChunkPair*>(m_ptr + m_pos);
        // uint64_t offset = m_pos + chunk_header_size;
        // uint8_t* start = m_ptr + offset;
        // uint64_t chunk_size = total_size - chunk_header_size;
        
        // chunk_pair->chunk = std::move(Chunk(start, chunk_size));
        // chunk_pair->state = ChunkState::IN_USE;
        
        // m_pos += total_size;

        // ChunkPair *pos = m_chunks;
        // for (; pos != nullptr; pos = pos->next) {

        // }
        // pos = chunk_pair;
        
        // new_chunk = &*m_chunks.insert(m_chunks.end(), {chunk, ChunkState::IN_USE});
    }

    // NOTE: If the old_chunk was specified, copy all the data from it to the new chunk.
    // Find its corresponding pair inside m_chunks list and set its state to ChunkState::FREE, 
    // so it can be reused later by others.
    if (old_chunk != nullptr) { 
        new_chunk->chunk.copy(old_chunk, old_chunk->occupied());
        ChunkPair* old_chunk_pair = m_chunks;
        for (; old_chunk_pair != nullptr; old_chunk_pair->next) {
            if ((&old_chunk_pair->chunk == old_chunk) && (old_chunk_pair->state == ChunkState::IN_USE))
                break;
        }
        assert(old_chunk_pair);
        old_chunk_pair->state = ChunkState::FREE;
        old_chunk_pair->chunk.reset();
    }

    return &new_chunk->chunk;
}

void Arena::releaseChunk(Chunk *chunk) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (chunk != nullptr) {
        ChunkPair* chunk_pair = m_chunks;
        for (; chunk_pair != nullptr; chunk_pair = chunk_pair->next) {
            if ((&chunk_pair->chunk == chunk) && (chunk_pair->state == ChunkState::IN_USE))
                break;
        }
        assert(chunk_pair);
        chunk_pair->state = ChunkState::FREE;
        chunk_pair->chunk.reset();
    }
}

void* Arena::allocMemory(uint64_t size) {
#ifdef _WIN32
    void *memory = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#else
    void *memory = malloc(size);
    std::memset(memory, 0, size);
#endif
    assert(memory);
    return memory;
}

void Arena::releaseMemory(void *memory) {
#ifdef _WIN32
    VirtualFree(memory, 0, MEM_RELEASE);
#else
    free(memory);
#endif
}

} // namespace mylib