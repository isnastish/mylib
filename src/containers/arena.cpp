#include "arena.h"

namespace ml
{
MemoryArena::MemoryArena(std::uint64_t size, std::uint64_t align) {
    uint64_t rem = size % align;
    uint64_t aligned_cap = size + (align - rem); 
    uint8_t *mem = static_cast<uint8_t *>(malloc(aligned_cap));
    std::memset(mem, 0, aligned_cap);
    m_ptr = mem;
    m_cap = aligned_cap;
    m_pos = 0;
    m_align = align;
}

MemoryArena::~MemoryArena() {
    free(m_ptr);
}

MemoryChunk* MemoryArena::get_memory_chunk(MemoryChunk *old_chunk, uint64_t size) {
    std::lock_guard<std::mutex> lock(mutex);

    // TODO: If right after the chunk comes m_pos, that means that we only have one chunk in a whole arena. 
    // And the chunk itself doesn't have enough memory, we have to extend the current chunk, 
    // rather than creating a new one.
    uint64_t new_size = (old_chunk == nullptr) ? size : (old_chunk->m_pos + size);
    uint64_t total_chunks = static_cast<uint64_t>((new_size / static_cast<float>(CHUNK_SIZE)) + 1);
    uint64_t total_size = total_chunks * CHUNK_SIZE;
    auto itr = std::find_if(chunks.begin(), chunks.end(), 
    [total_size](std::pair<MemoryChunk*, ChunkState>& chunk) -> bool {
        if ((chunk.second == ChunkState::Free) && (chunk.first->m_size >= total_size))
            return true;
        return false;
    });
    if (itr != chunks.end()) {
        MemoryChunk *chunk = itr->first;
        if (old_chunk != nullptr) {
            copy_chunk(chunk, old_chunk);
        }
        return chunk;
    }

    // NOTE: If this fails, we have to grow the arena, which would invalidate all the pointers.
    // Thus we cannot do the reallocation at this point in time.
    assert((total_size + sizeof(MemoryChunk)) <= (m_cap - m_pos));
    MemoryChunk *chunk_ptr = reinterpret_cast<MemoryChunk *>(m_ptr + m_pos);
    
    m_pos += sizeof(MemoryChunk);

    *chunk_ptr = MemoryChunk(m_ptr + m_pos, total_size);

    m_pos += total_size;

    chunks.push_back({chunk_ptr, ChunkState::InUse});

    if (old_chunk != nullptr) {
        copy_chunk(chunk_ptr, old_chunk);
    }

    return chunk_ptr;
}

void MemoryArena::release_memory_chunk(MemoryChunk *chunk) {
    std::lock_guard<std::mutex> lock(mutex);

    if (chunk != nullptr) {
        std::pair<MemoryChunk*, ChunkState>* pair = chunk_pair(chunk);
        assert(pair != nullptr);
        pair->second = ChunkState::Free;
        pair->first->m_pos = 0;
    }
}

void MemoryArena::copy_chunk(MemoryChunk *new_chunk, MemoryChunk *old_chunk) {
    std::pair<MemoryChunk*, ChunkState>* pair = chunk_pair(old_chunk);
    assert(pair != nullptr);
    std::memcpy(new_chunk->m_start, old_chunk->m_start, old_chunk->m_pos);
    pair->second = ChunkState::Free;
    pair->first->m_pos = 0;
}

std::pair<MemoryChunk*, ChunkState>* MemoryArena::chunk_pair(MemoryChunk *chunk) {
    auto chunk_itr = std::find_if(chunks.begin(), chunks.end(), 
    [chunk](const std::pair<MemoryChunk*, ChunkState>& pair) -> bool {
        if ((pair.first->m_start == chunk->m_start)
            && (pair.second == ChunkState::InUse))
            return true;
        return false;
    });
    return (chunk_itr == chunks.end()) ? nullptr : &*chunk_itr; 
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
        delete[]tmp;
    }
}

} // namespace ml