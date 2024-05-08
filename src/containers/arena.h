/*
*
*
*
*
*/

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
#include <memory> // std::unique_ptr
#include <optional> // std::optional

namespace mylib
{

class Chunk {
public:
    Chunk(std::byte* start, std::uint64_t size);

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
    const std::byte* begin() const { return m_start; }

    /**
     * @return A const pointer to the end of the memory chunk.
    */
    const std::byte* end() const { return (m_start + m_pos); }

    /**
     * @return A pointer to the beginning of the memory chunk. 
    */
    std::byte* begin() { return m_start; }

    /**
     * @return A pointer to the end of already occupied memory. 
    */
    std::byte* end() { return (m_start + m_pos); }

    /**
     * Compute the size of the remaining space in a chunk.
     * @return number of bytes remaining bytes.
    */
    std::uint64_t remaining() const { return m_size - m_pos; }
    
    /**
     * @return Total chunk size.
    */
    std::uint64_t size() const { return m_size; }

    /**
     * @return How much space is already occupied.
    */
    std::uint64_t occupied() const { return size() - remaining(); }

    /**
     * Copies the data residing in a source chunk, supplied as an argument
     * to the current chunk. Adjusts the position of the current chunk accordingly.
     * @param src_chunk Memory chunk to copy the data from.
     * @param size Size of the data to be copied.
    */
    void copy(const Chunk* src_chunk, uint64_t size);

    /**
     * 
    */
    void reset() { std::memset(m_start, 0, m_pos); m_pos = 0; }

private:
    std::byte*    m_start;
    std::uint64_t m_size;
    std::uint64_t m_pos;
};

class Arena {   
    class MemBlock {
    public:
        explicit MemBlock(std::uint64_t size);
        ~MemBlock();

        MemBlock(const MemBlock&) = delete;
        MemBlock& operator=(const MemBlock&) = delete;
        
        Chunk*                newChunk(std::uint64_t size);
        void                  freeChunk(Chunk* chunk);
        std::optional<Chunk*> getEmptyChunk(std::uint64_t size);
        std::uint64_t         totalChunks() const;
        std::uint64_t         emptyChunksCount() const;
        std::uint64_t         remainingSpace() const;

    private:
        friend class Arena;

        enum class ChunkState : std::uint8_t {
            IN_USE,
            FREE
        };
        
        struct ChunkHeader {
            Chunk      chunk;
            ChunkState state;
            ChunkHeader* next;
            ChunkHeader* prev;
        };
        
        static void* allocMemory(std::uint64_t size);
        static void  releaseMemory(void* memory);

        std::uint64_t m_pos = 0;
        std::uint64_t m_cap = 0;
        std::uint64_t m_total_chunks_count = 0;
        std::uint64_t m_empty_chunks_count = 0; 
        std::byte*    m_ptr = nullptr;
        ChunkHeader*  m_chunks = nullptr;
    };
    
public:
    Arena(std::uint64_t size=DEFAULT_ALLOC_SIZE);

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    
    Arena(Arena&& rhs); 
    Arena& operator=(Arena&& rhs);

    Chunk*        getChunk(uint64_t size, Chunk* old_chunk=nullptr);
    void          releaseChunk(Chunk* chunk);
    std::uint64_t emptyChunksCount() const;
    std::uint64_t totalChunks() const;

private:
    constexpr static std::uint64_t PAGE_SIZE{1024u};
    constexpr static std::uint64_t DEFAULT_ALLOC_SIZE{1024*1024*1024u};
    constexpr static std::uint64_t CHUNK_HEADER_SIZE{sizeof(MemBlock::ChunkHeader)};

    std::uint64_t                        m_blocks_count = 0;
    std::list<std::unique_ptr<MemBlock>> m_blocks;
    mutable std::mutex                   m_mutex;
};

} // namespace mylib