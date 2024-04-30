#include "arena.h"

namespace ml
{
void MemoryArena::realloc(uint64_t size) {
    uint64_t rem = m_cap - m_pos;
    if (size > rem) {
        uint64_t new_cap = std::max(size + (m_cap - rem), m_cap << 2);
        uint8_t *mem = new uint8_t[new_cap];
        std::memset(mem, 0, new_cap);
        std::memcpy(mem, m_ptr, m_pos);
        uint8_t *tmp = m_ptr;
        m_ptr = mem;
        m_cap = new_cap;
        delete[]tmp;
    }
}

MemoryArena::MemoryArena(std::uint64_t size, std::uint64_t align) {
    uint64_t rem = size % align;
    uint64_t aligned_cap = size + (align - rem); 
    uint8_t *mem = new uint8_t[aligned_cap];
    std::memset(mem, 0, aligned_cap);
    m_ptr = mem;
    m_cap = aligned_cap;
    m_pos = 0;
    m_align = align;
}

MemoryArena::~MemoryArena() {
    delete []m_ptr;
    std::memset(this, 0, sizeof(*this));
}

MemoryArena::MemoryArena(const MemoryArena& rhs) {
    m_align = rhs.m_align;
    m_cap = rhs.m_cap;
    m_pos = rhs.m_pos;
    m_ptr = new uint8_t[m_cap];
    std::memset(m_ptr, 0, m_cap);
    std::memcpy(m_ptr, rhs.m_ptr, rhs.m_pos);
}

MemoryArena& MemoryArena::operator=(const MemoryArena& rhs) {
    if (this == &rhs) return *this;
    uint8_t *old_ptr = m_ptr;
    m_align = rhs.m_align;
    m_cap = rhs.m_cap;
    m_pos = rhs.m_pos;
    m_ptr = new uint8_t[m_cap];
    std::memset(m_ptr, 0, m_cap);
    std::memcpy(m_ptr, rhs.m_ptr, rhs.m_pos);
    delete[]old_ptr;
    return *this;
}

MemoryArena::MemoryArena(MemoryArena&& rhs) {
    m_align = rhs.m_align;
    m_cap = rhs.m_cap;
    m_pos = rhs.m_pos;
    m_ptr = rhs.m_ptr;
    std::memset(&rhs, 0, sizeof(rhs));
}

MemoryArena& MemoryArena::operator=(MemoryArena&& rhs) {
    if (this == &rhs) return *this;
    uint8_t *old_ptr = m_ptr;
    m_align = rhs.m_align;
    m_cap = rhs.m_cap;
    m_pos = rhs.m_pos;
    m_ptr = rhs.m_ptr;
    std::memset(&rhs, 0, sizeof(rhs));
    delete[]old_ptr;
    return *this;
}

} // namespace ml