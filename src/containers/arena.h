#pragma once
#include <cstdint>
#include <cstring>
#include <iterator>
#include <list>
#include <mutex>

namespace ml
{

constexpr static std::uint64_t ALLOC_SIZE{4*1024*1024};

// TODO: Remove the alignment (make it optional)
class MemoryArena {
public:
    MemoryArena(std::uint64_t size=ALLOC_SIZE, std::uint64_t align=4);
    ~MemoryArena();

    MemoryArena(const MemoryArena& rhs);
    MemoryArena& operator=(const MemoryArena& rhs);
    MemoryArena(MemoryArena&& rhs);
    MemoryArena& operator=(MemoryArena&& rhs);

    // private?
    // template<class Object>
    // uint64_t getObjSize(const Object& obj) const {
    //     uint64_t obj_size = sizeof(obj);
    //     // uint64_t aligned_size = obj_size + (m_align - (obj_size % m_align));
    //     return aligned_size;
    // }

    template<class Object>
    void push(const Object& obj) {
        // uint64_t obj_aligned_size = getObjSize(obj);
        uint64_t obj_size = sizeof(obj);
        realloc(obj_size);
        *(Object *)(m_ptr + m_pos) = obj; // Or use std::memcpy
        m_pos += obj_size;
    }

    template<class Object>
    void push(Object&& obj) {
        // uint64_t obj_aligned_size = getObjSize(obj);
        uint64_t obj_size = sizeof(obj);
        realloc(obj_size);
        *reinterpret_cast<Object *>(m_ptr + m_pos) = std::move(obj); 
        m_pos += obj_size;
    }

    template<class Object>
    void pop() {
        uint64_t obj_size = sizeof(Object);
        m_pos -= obj_size;
        std::memset((m_ptr + m_pos), 0, obj_size);
    }

    void realloc(uint64_t size);

    uint8_t *m_ptr;
    uint64_t m_pos;
    uint8_t  m_align;
    uint64_t m_cap;
};

// Memory chunk
struct Chunk {
    uint8_t *start;
    uint64_t size;
    uint64_t pos;

    bool is_free() { 
        return pos == 0;
    }
};

struct ArenaList {
    template<class Object>
    Chunk *push(Chunk *chunk, const Object& obj, uint64_t obj_size) {
        // std::lock_guard<std::mutex> lock(mutex);
        if ((chunk->pos + obj_size) <= chunk->size) {
            *(Object *)(chunk->start + chunk->pos) = obj;
            chunk->pos += obj_size;
            return chunk;
        } else {
            for (auto itr = chunks.begin(); itr != chunks.end(); itr++) {
                if ((itr->size <= (chunk->pos + obj_size)) && itr->is_free()) {
                    // 1. Copy the data from the current chunk to a new one. 
                    Chunk *new_chunk = *itr;
                    std::memcpy(new_chunk->start, chunk->start, chunk->pos);
                    new_chunk->pos = chunk->pos;
                    *(Object *)(new_chunk->start + new_chunk->pos) = obj;
                    new_chunk->pos += obj_size;
                    // 2. Clear the current chunk so it can be reused later.
                    std::memset(chunk->start, 0, chunk->pos);
                    // 3. Return new chunk
                    return new_chunk;
                }
            }
            // Couldn't find an empty chunk, grow the arena, 
            // which would mean that all the chunks would have to be recomputed.
            // But we grow an arena by the same size.
        }
    }

    MemoryArena *arena;
    std::list<Chunk> chunks;
    std::mutex mutex;
};

} // namespace ml