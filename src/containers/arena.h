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

class Chunk {
public:
    Chunk(uint8_t *ptr, uint64_t size);

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
    void copy(const Chunk* src_chunk, uint64_t size) {
        assert(m_size > size);
        uint8_t* start = m_start + m_pos; 
        std::memcpy(start, src_chunk->m_start, size);
        m_pos += size;
    }

    /**
     * 
    */
    void reset() { std::memset(m_start, 0, m_pos); m_pos = 0; }

private:
    uint8_t  *m_start;
    uint64_t m_size;
    uint64_t m_pos;
    Arena    *m_arena;
};


class Arena {
    /**
     * @brief The memory arena maintains a list of all available chunks.
     * If chunk is in use, it's assumed to have `IN_USE` state.
     * If chunk is free, it has a corresponding `Free` state.
    */
    enum class ChunkState : uint8_t {
        IN_USE,
        FREE
    };

    struct ChunkPair {
        Chunk      chunk;
        ChunkState state;
        ChunkPair *next;
        ChunkPair *prev;
    };

public:
    constexpr static uint64_t PAGE_SIZE = 1024u;
    constexpr static uint64_t ALLOC_SIZE{1024*1024*1024u};

    /**
     * Allocate `size` bytes of memory
     * @note The size is aligned by the size of a single page `PAGE_SIZE`.
     * Thus, if the requested size is 256bytes, 1024 will be allocated instead.
     * @param size A size to be allocated, by default it's 1Gib.
    */
    explicit Arena(std::uint64_t size=ALLOC_SIZE);

    /**
     * Releases the memory allocated during the construction.
    */
    ~Arena();

    /**
     * @brief Memory arenas neither can be moved nor copied.
     * @note Move assignment operator intentionally made private for moving Arenas internally.
     * When move assigning the object, the arena that we assigning to is not initialized, 
     * usually it's a pointer which we dereference. 
     * @see getChunk function, this is the only place where it's used. 
    */
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    /**
     * Searches for a memory chunk of a sufficient size and returns it to the user.
     * A chunk, passed as a first argument can either be extended to fit the size, 
     * or put in the `FREE`(ed) state, in which case it can be reused by someone else later, 
     * and a new chunk will be returned.
     * @note The function doesn't allocate or deallocate memory.
     * @param chunk The current chunk which holds data.
     * @param size A desired size, if supplied chunk is not NULL, the total size would be computed
     * as a sum of the provided chunk plus size.
     * @return A new Chunk which has enough space to fit the desired size.
    */
    Chunk* getChunk(uint64_t size, Chunk* chunk=nullptr);

    /**
     * Puts the supplied chunk into `FREE`(ed) state so it can be used by others.
     * @note Doesn't free any memory.
     * @note Once the chunk is released, it no longer can be used by the one who released it.
     * @param chunk Chunk to be released.
    */
    void releaseChunk(Chunk* chunk);

    /**
     * @return Total size of the arena.
    */
    uint64_t capacity() const { return m_cap; }

    /**
     * @return The amount of bytes left (empty space), not considering `FREE` chunks.
    */
    uint64_t remaining() const { return m_cap - m_pos; }

    /**
     * @todo The operation has to be thread-safe.
     * @return Total amount of chunks, both `IN_USE` and `FREE`.
    */
    uint64_t chunksCount() const { return m_chunks_count; }

    /**
     * Computes the amount of empty chunks (chunks with a state `FREE`).
     * @todo The operation has to be thread-safe.
     * @return Empty chunks count.
    */
    uint64_t emptyChunksCount() const { return m_empty_chunks_count; } 

private:
    ChunkPair*   getChunkPair();
    static void* allocMemory(uint64_t size);
    static void  releaseMemory(void* memory);

    uint8_t*   m_ptr;
    uint64_t   m_pos;
    uint64_t   m_cap;
    uint64_t   m_chunks_count;
    uint64_t   m_empty_chunks_count; 
    ChunkPair* m_chunks;
};

class ArenaList {
public:
    ArenaList();

    ArenaList(const ArenaList&) = delete;
    ArenaList& operator=(const ArenaList&) = delete;
    
    ArenaList(ArenaList&& rhs); 
    ArenaList& operator=(ArenaList&& rhs);

    Chunk*   getChunk(uint64_t size, Chunk* old_chunk);
    void     releaseChunk(Chunk* chunk);
    uint64_t arenasCount() const; 

private:
    std::unique_ptr<Arena> m_arenas;
    uint64_t               m_arenas_count;
    mutable std::mutex     m_mutex;
};

#if 0
//////////////////////////////////////////////////////////////////////////////////
// Usage of the API
template<class Object>
class Array {
public:
    explicit Array(ArenaList* arena_list, size_t count=0);
    Array(const Array& rhs);

    void pushBack(const Object& obj);
    void pushBack(Object&& obj);

private:
    ArenaList* m_arenas;
    Chunk*     m_chunk;
};


class String {
public:
    explicit String(ArenaList* arena_list);
    String(ArenaList* arena_list, std::string&& src);
    String(ArenaList* arena_list, const std::string& src);

    String& operator+=(const String&);
    String& operator+=(const std::string&);
    String& operator+=(const char*);
    String& operator+=(const std::string_view);

private:
    ArenaList* m_arenas;
    Chunk*     m_chunk;
};

String operator+(const String& a, const String& b) {

}

String operator+(const String& a, const std::string& b) {

}

String operator+(const String& a, std::string_view b) {

}

String operator+(const String& a, const char *b) {

}
#endif

} // namespace mylib