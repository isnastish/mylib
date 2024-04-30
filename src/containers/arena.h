#pragma once
#include <cstdint>
#include <cstring>
#include <iterator>

namespace ml
{

constexpr static std::uint64_t ALLOC_SIZE{4*1024*1024};

class MemoryArena {
public:
    MemoryArena(std::uint64_t size=ALLOC_SIZE, std::uint64_t align=4);
    ~MemoryArena();

    MemoryArena(const MemoryArena& rhs);
    MemoryArena& operator=(const MemoryArena& rhs);
    MemoryArena(MemoryArena&& rhs);
    MemoryArena& operator=(MemoryArena&& rhs);

    // private?
    template<class Object>
    uint64_t getObjSize(const Object& obj) const {
        uint64_t obj_size = sizeof(obj);
        uint64_t aligned_size = obj_size + (m_align - (obj_size % m_align));
        return aligned_size;
    }

    template<class Object>
    void push(const Object& obj) {
        uint64_t obj_aligned_size = getObjSize(obj);
        realloc(obj_aligned_size);
        *(Object *)(m_ptr + m_pos) = obj; // Or use std::memcpy
        m_pos += obj_aligned_size;
    }

    template<class Object>
    void push(Object&& obj) {
        uint64_t obj_aligned_size = getObjSize(obj);
        realloc(obj_aligned_size);
        *(Object *)(m_ptr + m_pos) = std::move(obj);
        m_pos += obj_aligned_size;
    }

    template<class Object>
    void pop() {
        uint64_t obj_aligned_size = sizeof(Object);
        m_pos -= obj_aligned_size;
        std::memset((m_ptr + m_pos), 0, obj_aligned_size);
    }

private:
    void realloc(uint64_t size);

    uint8_t *m_ptr;
    uint64_t m_pos;
    uint8_t  m_align;
    uint64_t m_cap;
};

// class TempMemoryArena {
// public:
// private:
//     MemoryArena *m_arena;
//     uint8_t *m_start;
// };

} // namespace ml