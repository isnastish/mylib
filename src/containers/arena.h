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

    /**
     * Compute the size of the remaining space in a chunk.
     * @return number of bytes remaining bytes.
    */
    uint64_t remaining() const { return m_size - m_pos; }
    
    /**
     * 
    */
    template<class Object>
    void push(const Object& obj, uint64_t size) {
        assert((m_pos + size) <= m_size);
        *(Object *)(m_start + m_pos) = obj;
        m_pos += size;
    }

    /**
     * 
    */
    template<class Object>
    void push(Object&& obj, uint64_t size) {
        assert((m_pos + size) <= m_size);
        *(Object *)(m_start + m_pos) = std::move(obj);
        m_pos += size;
    }

    /**
     * 
    */
    void pop(uint64_t size) { m_pos -= size; }

    /**
     * 
    */
    uint8_t* begin() { return m_start; }

    /**
     * 
    */
    uint8_t* end() { return (m_start + m_pos); }
  
private:
    friend class MemoryArena;

    uint8_t *m_start;
    uint64_t m_size;
    uint64_t m_pos;
};

class MemoryArena {
    constexpr static uint64_t CHUNK_SIZE = 1024u;
    constexpr static uint64_t ALLOC_SIZE{1024*1024*1024u};

    using ChunkPair = std::pair<MemoryChunk*, ChunkState>;
public:
    /**
     * 
    */
    MemoryArena(std::uint64_t size=ALLOC_SIZE, std::uint64_t align=4);

    /**
     * 
    */
    ~MemoryArena();

    MemoryArena(const MemoryArena& rhs) = delete;
    MemoryArena& operator=(const MemoryArena& rhs) = delete;
    MemoryArena(MemoryArena&& rhs) = delete;
    MemoryArena& operator=(MemoryArena&& rhs) = delete;

    /**
     * 
    */
    MemoryChunk* get_memory_chunk(MemoryChunk *old_chunk, uint64_t size);

    /**
     * 
    */
    void release_memory_chunk(MemoryChunk* chunk);

private:
    static void *alloc_memory(uint64_t size);
    static void release_memory(void *memory);
    void realloc(uint64_t size);
    void copy_chunk(MemoryChunk* new_chunk, MemoryChunk* old_chunk);
    ChunkPair* chunk_pair(MemoryChunk *chunk);

    uint8_t *m_ptr;
    uint64_t m_pos;
    uint8_t  m_align;
    uint64_t m_cap;
    std::list<ChunkPair> m_chunks;
    std::mutex m_mutex;
};

} // namespace ml