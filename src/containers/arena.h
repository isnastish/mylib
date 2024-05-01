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

/**
 * The memory arena maintains a list of all available chunks.
 * If chunks is in used, it's assumed to have `InUse` state.
 * If chunks is free, it has a corresponding `Free` state.
*/
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
     * Push an object into the memory, incrementing the current position.
     * Asserts if the current position plus size greater than memory size.
     * @param obj An object to be inserted.
     * @param size Object's size.
    */
    template<class Object>
    void push(const Object& obj, uint64_t size) {
        assert((m_pos + size) <= m_size);
        *(Object *)(m_start + m_pos) = obj;
        m_pos += size;
    }

    /**
     * The same as the function above, but the object is std::move(ed) into the memory.
     * @param obj An object to be moved.
     * @param size Object's size.
    */
    template<class Object>
    void push(Object&& obj, uint64_t size) {
        assert((m_pos + size) <= m_size);
        *(Object *)(m_start + m_pos) = std::move(obj);
        m_pos += size;
    }

    /**
     * Pop element from the memory by decrementing the position.
     * Asserts if `m_pos` less than size.
     * @param size Size of the previously inserted object.
    */
    void pop(uint64_t size) { 
        assert(m_pos >= size);
        m_pos -= size;
    }

    /**
     * @return Return a pointer to the beginning of the memory chunk. 
    */
    uint8_t* begin() { return m_start; }

    /**
     * @return Return a pointer to the end of already occupied memory. 
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
     * @param size A size to be allocated, by default it's 1Gib.
     * @param align Alignment is currently disabled. 
    */
    MemoryArena(std::uint64_t size=ALLOC_SIZE, std::uint64_t align=4);

    /**
     * Releases the memory allocated during the construction.
    */
    ~MemoryArena();

    /**
     * Memory arenas can neither be copied nor moved.
    */
    MemoryArena(const MemoryArena& rhs) = delete;
    MemoryArena& operator=(const MemoryArena& rhs) = delete;
    MemoryArena(MemoryArena&& rhs) = delete;
    MemoryArena& operator=(MemoryArena&& rhs) = delete;

    /**
     * Searches for a memory chunk of a sufficient size and returns it to the user.
     * A chunk, passed as a first argument can either be extended to fit the size, 
     * or put in the `Free`(ed) state, in which case it can be reused by someone else later, 
     * and a new chunk will be returned.
     * @note The function doesn't allocate or deallocate memory.
     * @param chunk The current chunk which holds data.
     * @param size A desired size, if supplied chunk is not NULL, the total size would be computed
     * as a sum of the provided chunk plus size.
     * @return A new MemoryChunk which has enough space to fit the desired size.
    */
    MemoryChunk* get_memory_chunk(MemoryChunk *chunk, uint64_t size);

    /**
     * Puts the supplied chunk into `Free`(ed) state so it can be used by others.
     * @note Doesn't free any memory.
     * @param chunk Chunk to be released.
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