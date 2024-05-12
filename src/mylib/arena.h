#pragma once

#include <cstdint>
#include <list>
#include <mutex>
#include <memory> // std::unique_ptr
#include <optional> // std::optional

namespace mylib
{
class Chunk {
public:
    /**
     * Push an object into the memory incrementing the current position.
     * @throw std::length_error If not enough space for placing the object. 
     * @param obj An object to be inserted.
     * @param size Object's size.
    */
    template<typename Object>
    void push(const Object& obj) {
        doesFit(sizeof(obj));
        *reinterpret_cast<Object*>(m_start + m_pos) = obj;
        m_pos += sizeof(obj);
    }

    /**
     * The same as the function above, but the object is std::move(ed) into the memory.
     * @throw std::length_error If not enough space for placing the object. 
     * @param obj An object to be moved.
     * @param size Object's size.
    */
    template<typename Object>
    void push(Object&& obj) {
        doesFit(sizeof(obj));
        *reinterpret_cast<Object*>(m_start + m_pos) = std::move(obj);
        m_pos += sizeof(obj);
    }

    /**
     * Pop element from the memory by decrementing the position.
     * @throw std::length_error If trying to pop on an empty chunk. 
     * @param size Size of the previously inserted object.
    */
    void pop(std::uint64_t size);

    /**
     * @return A pointer to the beginning of the memory chunk. 
    */
    std::byte* begin() noexcept { return m_start; }

    /**
     * @return A pointer to the end of already occupied memory. 
    */
    std::byte* end() noexcept { return m_start + m_pos; }

    /**
     * Compute the size of the remaining space in a chunk.
     * @return number of bytes remaining bytes.
    */
    std::uint64_t remainingSpace() const noexcept { return m_size - m_pos; }
    
    /**
     * @return Total chunk size.
    */
    std::uint64_t size() const noexcept { return m_size; }

    /**
     * Copies the data residing in a source chunk, supplied as an argument
     * to the current chunk. Adjusts the position of the current chunk accordingly.
     * @param src_chunk Memory chunk to copy the data from.
    */
    void copy(const Chunk* src_chunk);

    /**
     * 
    */
    void reset() noexcept;

private:
    friend class Arena;

    Chunk(std::byte* start, std::uint64_t size) noexcept;
    void doesFit(std::uint64_t size) const;

    std::byte*    m_start;
    std::uint64_t m_size;
    std::uint64_t m_pos;
};

class Arena {
    class MemBlock {
        /**
         * 
        */
        enum class ChunkState : std::uint8_t {
            IN_USE,
            FREE
        };
        
        /**
         * 
        */
        struct ChunkHeader {
            Chunk        chunk;
            ChunkState   state;
            ChunkHeader* next;
            ChunkHeader* prev;
        };
    public:
        explicit MemBlock(std::uint64_t size);
        ~MemBlock();

        MemBlock(const MemBlock&) = delete;
        MemBlock& operator=(const MemBlock&) = delete;
        
        Chunk*                newChunk(std::uint64_t size) noexcept;
        bool                  freeChunk(Chunk* chunk) noexcept;
        std::optional<Chunk*> getEmptyChunk(std::uint64_t size) noexcept;
        std::uint64_t         totalChunks() const noexcept;
        std::uint64_t         emptyChunksCount() const noexcept;
        std::uint64_t         remainingSpace() const noexcept;

    private:
        friend class Arena;

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
    constexpr static std::uint64_t CHUNK_HEADER_SIZE{sizeof(MemBlock::ChunkHeader)};
    constexpr static std::uint64_t PAGE_SIZE{1024u};
    constexpr static std::uint64_t DEFAULT_ALLOC_SIZE{1024*1024*1024u};
    
    Arena(std::uint64_t size=DEFAULT_ALLOC_SIZE);

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    
    Arena(Arena&& rhs); 
    Arena& operator=(Arena&& rhs);

    Chunk*        getChunk(std::uint64_t size, Chunk* old_chunk=nullptr);
    void          releaseChunk(Chunk* chunk);
    std::uint64_t emptyChunksCount() const;
    std::uint64_t totalChunks() const;
    std::uint64_t totalBlocks() const;

private:
    std::uint64_t                        m_blocks_count = 0;
    std::list<std::unique_ptr<MemBlock>> m_blocks;
    mutable std::mutex                   m_mutex;
};

} // namespace mylib