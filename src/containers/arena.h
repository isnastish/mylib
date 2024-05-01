#pragma once

#include <cstdint>
#include <cstring>
#include <iterator>
#include <list>
#include <mutex>
#include <cassert>
#include <type_traits>

namespace ml
{

enum class ChunkState : uint8_t {
    InUse,
    Free
};

class MemoryChunk {
public:
    MemoryChunk(uint8_t *ptr, uint64_t size)
    : m_start(ptr), m_size(size), m_pos(0) {}

    uint64_t left() const { return m_size - m_pos; }
private:
    friend class MemoryArena;

    uint8_t *m_start;
    uint64_t m_size;
    uint64_t m_pos;
};

class MemoryArena {
    constexpr static uint64_t CHUNK_SIZE = 1024u;
    constexpr static uint64_t ALLOC_SIZE{2*1024*1024u};

public:
    MemoryArena(std::uint64_t size=ALLOC_SIZE, std::uint64_t align=4);
    ~MemoryArena();

    MemoryArena(const MemoryArena& rhs) = delete;
    MemoryArena& operator=(const MemoryArena& rhs) = delete;
    MemoryArena(MemoryArena&& rhs) = delete;
    MemoryArena& operator=(MemoryArena&& rhs) = delete;

    /*
    *
    */
    MemoryChunk* get_memory_chunk(MemoryChunk *old_chunk, uint64_t size);

    /*
    * 
    */
    void release_memory_chunk(MemoryChunk* chunk);

    /*
    *
    */
    void realloc(uint64_t size);

private:
    void copy_chunk(MemoryChunk* new_chunk, MemoryChunk* old_chunk);
    std::pair<MemoryChunk*, ChunkState>* chunk_pair(MemoryChunk *chunk);

    uint8_t *m_ptr;
    uint64_t m_pos;
    uint8_t  m_align;
    uint64_t m_cap;

    std::list<std::pair<MemoryChunk*, ChunkState>> chunks;
    std::mutex mutex;
};

} // namespace ml