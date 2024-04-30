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

    template<class Object>
    void push(const Object& obj) {
        std::uint64_t obj_size = sizeof(obj);
        std::uint64_t rem = m_cap - std::distance(m_ptr, m_pos); 
        if (obj_size > rem) {
            // uint8_t *old_ptr = m_ptr;
            uint64_t new_cap = std::max(obj_size + (m_cap - rem), m_cap << 2);
            uint64_t aligned_cap = new_cap + (m_align - (new_cap % m_align));
            uint8_t *mem = new uint8_t[aligned_cap];
            std::memset(mem, 0, aligned_cap);
            std::memcpy(mem, m_ptr, std::distance(m_ptr, m_pos));
            m_ptr = mem;
            m_cap = aligned_cap;
            
        }
        reinterpret_cast<Object *>(m_pos) = obj;
        m_pos += obj_size;
    }

    template<class Object>
    void push(Object&& obj) {
        // Not implemented yet.
        __debugbreak();
    }

private:
    uint8_t *m_ptr;
    uint8_t *m_pos;
    uint8_t m_cap;
    uint8_t m_align;
};

MemoryArena::MemoryArena(std::uint64_t size, std::uint64_t align) {
    auto rem = size % align;
    auto aligned_size = size + (align - rem); 
    uint8_t *mem = new uint8_t[aligned_size];
    std::memset(mem, 0, aligned_size);
    m_ptr = m_pos = mem;
    m_cap = aligned_size;
    m_align = align;
}

MemoryArena::~MemoryArena() {
    delete []m_ptr;
    std::memset(this, 0, sizeof(*this));
}

// template<class Object>
// class Vector {
// private:
//     MemoryArena *m_arena;
// };

// class TempMemoryArena {
// private:
//     MemoryArena *arena;
//     uint8_t *start;
//     uint8_t *end;
// };

} // namespace ml