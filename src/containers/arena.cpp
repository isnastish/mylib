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

    // NOTE: Compute a total allocation size taking into account an alignment.
    // If a chunk is nullptr, we include the size of the MemoryChunk into the total allocation size.
    // Thus, if requested size is equal to 1024, which is exactly the size of a single page (PAGE_SIZE), 
    // two pages will be allocated, comprising the size of 2048bytes in total,
    // because MemoryChunk sturuct occupies 24bytes, thus 1024+24 = 1048 aligned by PAGE_SIZE would be 2048, or 2*PAGE_SIZE.
    // +--------+-------------------+------------------------------------------------------------+
    // | chunk  |         size      |                      empty space                           |
    // | header |         size      |                      empty space                           |
    // +--------+-------------------+------------------------------------------------------------+
    //                              ^
    //                              |
    //                             m_pos
    uint64_t chunk_header_size = sizeof(MemoryChunk);
    bool extend_current_chunk = (old_chunk != nullptr) && (m_pos == (old_chunk->m_size + chunk_header_size));
    uint64_t new_size = size + (extend_current_chunk ? 0 : chunk_header_size);
    uint64_t rem = new_size % PAGE_SIZE;
    uint64_t new_aligned_size = (rem == 0) ? new_size : (new_size + (PAGE_SIZE - (new_size % PAGE_SIZE)));
    uint64_t page_count = new_aligned_size / PAGE_SIZE;
    uint64_t total_size = page_count * PAGE_SIZE;

    // NOTE: Extend the current chunk rather than reassigning a new one.
    if (extend_current_chunk) {
        assert(total_size <= remaining());
        m_pos += total_size;
        old_chunk->m_size += total_size;
        return old_chunk;
    }

    // NOTE: When looking for a new chunk of memory, we don't just grab the first which satisfies our size requirement.
    // What we do instead is to search for the chunk with size closest to what we want.
    // As we iterate through all the chunks, we compare their sizes and reassign new_chunk pointer if more optimal chunk 
    // has been found.
    // The const of this operation is O(N), where N is the number of chunks stored in a chunks list, 
    // plus the time spend on a size comparison.
    ChunkPair* new_chunk = nullptr;
    for (auto itr = m_chunks.begin(); itr != m_chunks.end(); itr++) {
        if ((itr->second == ChunkState::Free) && (itr->first->size() >= (total_size - chunk_header_size))) {
            if (new_chunk == nullptr) {
                new_chunk = &*itr;
            }
            else {
                if (itr->first->size() < new_chunk->first->size()) {
                    new_chunk = &*itr;
                }
            }
        }
    }

    if (new_chunk != nullptr) {
        new_chunk->second = ChunkState::InUse;
    }
    else {
        // NOTE: A chunk of a suitable size hasn't been found, 
        // so allocate a new one out of empty space.
        assert(total_size <= remaining());
        MemoryChunk *chunk = (MemoryChunk *)(m_ptr + m_pos);
        uint64_t offset = m_pos + chunk_header_size;
        uint8_t* start = m_ptr + offset;
        uint64_t chunk_size = total_size - chunk_header_size;
        *chunk = MemoryChunk(start, chunk_size);
        m_pos += total_size;
        new_chunk = &*m_chunks.insert(m_chunks.end(), {chunk, ChunkState::InUse});
    }

    // NOTE: If the old_chunk was specified, copy all the data from it to the new chunk.
    // Find its corresponding pair inside m_chunks list and set its state to ChunkState::Free, 
    // so it can be reused by others.
    if (old_chunk != nullptr) { 
        uint64_t copy_size = old_chunk->occupied();
        std::memcpy(new_chunk->first->m_start, old_chunk->m_start, copy_size);
        new_chunk->first->m_pos += copy_size;

        // Find the old chunk's pair and set its state to Free
        auto old_chunk_pair = std::find_if(m_chunks.begin(), m_chunks.end(),
        [old_chunk](const std::pair<MemoryChunk*, ChunkState>& chunk_pair) -> bool {
            return (chunk_pair.first == old_chunk) && (chunk_pair.second == ChunkState::InUse);
        });
        assert(old_chunk_pair != m_chunks.end());
        old_chunk_pair->second = ChunkState::Free;
        std::memset(old_chunk_pair->first->m_start, 0, copy_size);
        old_chunk_pair->first->m_pos = 0;
    }

    return new_chunk->first;
}

void MemoryArena::release_memory_chunk(MemoryChunk *chunk) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (chunk != nullptr) {
        ChunkPair& pair = find_chunk_pair(chunk);
        pair.second = ChunkState::Free;
        std::memset(pair.first->m_start, 0, pair.first->m_pos);
        pair.first->m_pos = 0;
    }
}

MemoryArena::ChunkPair& MemoryArena::find_chunk_pair(MemoryChunk* chunk) {
    assert(chunk != nullptr);
    auto chunk_pair = std::find_if(m_chunks.begin(), m_chunks.end(),
    [chunk](const std::pair<MemoryChunk*, ChunkState>& chunk_pair) -> bool {
        return (chunk_pair.first == chunk) && (chunk_pair.second == ChunkState::InUse);
    });
    assert(chunk_pair != m_chunks.end());
    return *chunk_pair;
}

uint64_t MemoryArena::total_chunks_count() const { 
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_chunks.size(); 
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