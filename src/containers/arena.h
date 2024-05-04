#pragma once

#include <cstdint>
#include <cstring>
#include <iterator>
#include <list>
#include <mutex>
#include <cassert>
#include <type_traits>
#include <algorithm>
#include <utility>

namespace mylib
{

/**
 * The memory arena maintains a list of all available chunks.
 * If chunk is in use, it's assumed to have `InUse` state.
 * If chunk is free, it has a corresponding `Free` state.
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
     * @return A const pointer to the beginning of the memory chunk.  
    */
    const uint8_t* begin() const { return m_start; }

    /**
     * @return A const pointer to the end of the memory chunk.
    */
    const uint8_t* end() const { return (m_start + m_pos); }

    /**
     * @return A pointer to the beginning of the memory chunk. 
    */
    uint8_t* begin() { return m_start; }

    /**
     * @return A pointer to the end of already occupied memory. 
    */
    uint8_t* end() { return (m_start + m_pos); }

    /**
     * Compute the size of the remaining space in a chunk.
     * @return number of bytes remaining bytes.
    */
    uint64_t remaining() const { return m_size - m_pos; }
    
    /**
     * @return Total chunk size.
    */
    uint64_t size() const { return m_size; }

    /**
     * @return How much space is already occupied.
    */
    uint64_t occupied() const { return size() - remaining(); }

    /**
     * Copies the data residing in a source chunk, supplied as an argument
     * to the current chunk. Adjusts the position of the current chunk accordingly.
     * @param src_chunk Memory chunk to copy the data from.
     * @param size Size of the data to be copied.
    */
    void copy(const MemoryChunk* src_chunk, uint64_t size) {
        assert(remaining() > size);
        uint8_t* start = m_start + m_pos; 
        std::memcpy(start, src_chunk->begin(), size);
        m_pos += size;
    }
    
private:
    friend class MemoryArena;

    uint8_t *m_start;
    uint64_t m_size;
    uint64_t m_pos;
};

class MemoryArena {
    using ChunkPair = std::pair<MemoryChunk*, ChunkState>;
public:
    constexpr static uint64_t PAGE_SIZE = 1024u;
    constexpr static uint64_t ALLOC_SIZE{1024*1024*1024u}; // 1gib

    /**
     * Allocate `size` bytes of memory
     * @note The size is aligned by the size of a single page `PAGE_SIZE`.
     * Thus, if the requested size is 256bytes, 1024 will be allocated instead.
     * @param size A size to be allocated, by default it's 1Gib.
    */
    MemoryArena(std::uint64_t size=ALLOC_SIZE);

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
    MemoryChunk* get_memory_chunk(uint64_t size, MemoryChunk* chunk=nullptr);

    /**
     * Puts the supplied chunk into `Free`(ed) state so it can be used by others.
     * @note Doesn't free any memory.
     * @note Once the chunk is released, it no longer can be used by the one who released it.
     * @param chunk Chunk to be released.
    */
    void release_memory_chunk(MemoryChunk* chunk);

    /**
     * @return Total size of the arena.
    */
    uint64_t capacity() const { return m_cap; }

    /**
     * @return The amount of bytes left (empty space), not considering `Free` chunks.
    */
    uint64_t remaining() const { return m_cap - m_pos; }

    /**
     * @return Total amount of chunks, both `InUse` and `Free`.
    */
    uint64_t total_chunks_count() const;

    /**
     * Computes the amount of empty chunks (chunks with a state `Free`).
     * @note This operation involves locking the mutex.
     * @return Empty chunks count.
    */
    uint64_t empty_chunks_count() const; 

private:
    static void *alloc_memory(uint64_t size);
    static void release_memory(void *memory);
    ChunkPair* find_chunk_pair(MemoryChunk* chunk);

    uint8_t *m_ptr;
    uint64_t m_pos;
    uint64_t m_cap;
    std::list<ChunkPair> m_chunks;
    mutable std::mutex m_mutex;
};

} // namespace mylib