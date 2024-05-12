#pragma once

#include "arena.h"
#include <fmt/core.h>
#include <utility>
#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace mylib
{
namespace detail
{
class invalid_iterator_error : public std::exception {
public:
    explicit invalid_iterator_error(const char* m)
    : std::exception(m)
    {}
};
}

template<class Object>
class GrowingArray {
public:
    /**
     * 
    */
    class const_iterator;

    /**
     * 
    */
    class iterator;

    /**
     * 
    */
    explicit GrowingArray(Arena* arena) noexcept
     : m_arena(arena), m_chunk(nullptr), m_size(0), m_cap(0) {}

    /**
     * 
    */
    ~GrowingArray() noexcept {
        if (m_chunk) m_arena->releaseChunk(m_chunk);
    }

    /**
     * 
    */
    GrowingArray(const GrowingArray<Object>& rhs) noexcept {
        m_arena = rhs.m_arena;
        if (rhs.m_chunk) {
            m_chunk = rhs.m_arena->getChunk(rhs.m_chunk->size());
            m_chunk->copy(rhs.m_chunk);
            m_size = rhs.m_size;
        }
    }

    /**
     * 
    */
    GrowingArray& operator=(const GrowingArray<Object>& rhs) noexcept { init(rhs); return *this; }

    /**
     * 
    */
    GrowingArray(GrowingArray&& rhs) noexcept {
        m_arena = rhs.m_arena; m_chunk = rhs.m_chunk; m_size = rhs.m_size;
        rhs.m_chunk = nullptr; rhs.m_size = rhs.m_cap = 0;
    }

    /**
     * 
    */
    GrowingArray& operator=(GrowingArray&& rhs) noexcept {
        init(rhs); rhs.m_chunk = nullptr; 
        rhs.m_size = rhs.m_cap = 0;
        return *this;
    }

    /**
     * 
    */
    const Object& operator[](int index) const { validateIndex(index); Object* begin = reinterpret_cast<Object*>(m_chunk->begin()); return begin[index]; }

    /**
     * 
    */
    Object& operator[](int index) { validateIndex(index); Object* begin = reinterpret_cast<Object*>(m_chunk->begin()); return begin[index]; }

    /**
     * 
    */
    bool grow(std::uint64_t new_cap) noexcept {
        const std::uint64_t size = sizeof(Object)*new_cap;
        if (!new_cap) return false;
        if (!m_chunk) { m_chunk = m_arena->getChunk(size); m_cap = new_cap; return true; } 
        else if (size > m_chunk->remainingSpace()) {  m_chunk = m_arena->getChunk(m_chunk->size()*2, m_chunk); m_cap = new_cap; return true; }
        return false;
    }

    /**
     * 
    */
    void push_back(const Object& value) noexcept { grow(1); m_chunk->push(value); m_size += 1; }

    /**
     * 
    */
    void push_back(Object&& value) noexcept { grow(1); m_chunk->push(std::move(value)); m_size += 1; }

    /**
     * 
    */
    const Object& back() const { validateIndex(m_size - 1); Object* data = reinterpret_cast<Object*>(m_chunk->begin()); return data[m_size-1]; }

    /**
     * 
    */
    const Object& front() const { validateIndex(0); Object* data = reinterpret_cast<Object*>(m_chunk->begin()); return data[0]; }

    /**
     * 
    */
    void pop_back() { validateIndex(m_size - 1); m_chunk->pop(sizeof(Object)); m_size -= 1; }

    /**
     * 
    */
    std::size_t size() const noexcept { return m_size; } 

    /**
     * 
    */
    std::size_t capacity() const noexcept { return m_cap; }

    /**
     * 
    */
    bool empty() const noexcept { return m_size == 0; }

    /**
     * 
    */
    std::size_t max_size() const noexcept { return m_cap - m_size; }

    /**
     * 
    */
    void clear() noexcept { if (m_chunk) { m_chunk->reset(); } m_size = 0; }

    /**
     *
    */
    const_iterator begin() const { 
        if (m_chunk) return const_iterator(reinterpret_cast<Object*>(m_chunk->begin()));
        return const_iterator(); 
    }

    /**
     * 
    */
    const_iterator end() const { 
        if (m_chunk) const_iterator(reinterpret_cast<Object*>(m_chunk->end()));
        return const_iterator(); 
    }

    /**
     * 
    */
    iterator begin() { 
        if (m_chunk) return iterator(reinterpret_cast<Object*>(m_chunk->begin()));
        return iterator(); 
    }

    /**
     * 
    */
    iterator end() { 
        if (m_chunk) return iterator(reinterpret_cast<Object*>(m_chunk->end()));
        return iterator(); 
    }

    /**
     * 
    */
    iterator insert(const_iterator pos, const Object& value) {

    }

    /**
     * 
    */
    template<class ...Args>
    iterator emplace(const_iterator pos, Args&&...args) {
        return insert(pos, Object(std::forward<Args>(args)...));
    }

#if 0
    iterator erase(iterator pos);
    iterator erase(iterator first, iterator second);

    // TODO: Implement
    iterator erase(const_iterator pos);
    iterator erase(const_iterator first, const_iterator second);

    iterator insert(const_iterator pos, const Object& obj);
    iterator insert(const_iterator pos, Object&& obj);
    iterator insert(const_iterator pos, size_t count, const Object& obj);

    template<class Itr>
    iterator insert(const_iterator pos, Itr first, Itr last)
    {
        auto count = std::abs(std::distance(first, last));
        auto at = shift(pos, count);
        for (auto itr = first; itr != last;)
            *at++ = *itr++;
        m_size += count;
        return at;
    }
#endif

private:
    void validateIndex(int index) const {
        if ((!m_chunk) || (index < 0 || index >= m_size)) 
            throw std::out_of_range(fmt::format("index {} is out of range", index));
    }

    void init(const GrowingArray& rhs) noexcept {
        if (this == &rhs) return *this;
        if (m_chunk) m_arena->releaseChunk(m_chunk);
        if (rhs.m_chunk) {
            m_chunk = m_arena->getChunk(rhs.m_chunk->size());
            m_chunk->copy(rhs.m_chunk);
            m_size = rhs.m_size;
        }
    }

    Arena*      m_arena;
    Chunk*      m_chunk;
    std::size_t m_size;
    std::size_t m_cap;
};

template<typename Object>
class GrowingArray<Object>::const_iterator {
public:
    const_iterator() : m_object(nullptr) {}
    
    const Object& operator*() const            { validate(); return static_cast<const Object&>(*m_object); }
    const_iterator& operator++()               { validate(); m_object++; return *this; }
    const_iterator operator++(int /*postfix*/) { const_iterator old_this = *this; ++(*this); return old_this; }
    const_iterator operator--()                { validate(); m_object--; return *this; }
    const_iterator operator--(int /*postfix*/) { const_iterator old_this = *this; --(*this); return old_this; }

    bool operator==(const const_iterator& rhs) const noexcept { return (m_object == rhs.m_object); }
    bool operator!=(const const_iterator& rhs) const noexcept { return !(*this == rhs); }

protected:
    friend class GrowingArray<Object>;
    void validate() const { if (!m_object) throw detail::invalid_iterator_error("invalid iterator"); }
    const_iterator(Object* object) 
    : m_object(object) {}

    Object* m_object;
    // For validation (not used yet)
    // Chunk** m_chunkp;
    // Chunk* m_old_chunk;
};

template<class Object>
class GrowingArray<Object>::iterator : public const_iterator {
public:
    iterator() {}

    Object& operator*()                  { return const_cast<Object&>(const_iterator::operator*()); }
    const Object& operator*() const      { return const_iterator::operator*(); }
    iterator& operator++()               { const_iterator::validate(); const_iterator::m_object++; return *this; }
    iterator operator++(int /*postfix*/) { iterator old_this = *this; ++(*this); return old_this; }
    iterator& operator--()               { const_iterator::validate(); const_iterator::m_object--; return *this; }
    iterator operator--(int /*postfix*/) { iterator old_this = *this; --(*this); return old_this; }

protected:
    friend class GrowingArray<Object>;

    iterator(Object* object) : const_iterator(object) {}
};

#if 0
template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::erase(iterator pos) 
{
    assert(pos >= begin() && pos < end());
    auto next = pos;
    for (; next != (end() - 1); next++)
        std::swap(*next, *(next + 1));
    m_size--;
    return pos;
}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::erase(iterator first, iterator last)
{
    assert((first >= begin() && first <= end()) &&
    (last >= begin() && last <= end()));
    size_t erase_size = std::distance(first, last);
    for (auto at = last; at != end();)
        *first++ = *at++;
    m_size -= erase_size;
    return first;
}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::erase(const_iterator first, const_iterator last)
{
    assert((first >= begin() && first <= end()) &&
    (last >= begin() && last <= end()));
    auto erase_size = std::distance(first, last);
    auto erase_at = const_cast<iterator>(first); // Does it have to be a const_cast?
    for (auto at = last; at != end();)
        *erase_at++ = *at++;
    m_size -= erase_size;
    return first;
}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::insert(const_iterator pos, const Object& obj)
{
    auto at = shift(pos, 1);
    *at = obj;
    m_size++;
    return at;
}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::insert(const_iterator pos, Object&& obj)
{
    auto at = shift(pos,  1);
    *at = std::move(obj);
    m_size++;
    return at;
}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::insert(const_iterator pos, size_t count, const Object& obj)
{
    auto at = shift(pos, count);
    for (size_t i = 0; i < count; i++)
        *at++ = obj;
    m_size += count;
    return at;
}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::shift(const_iterator pos, size_t count)
{
    assert(pos >= begin() && pos <= end());
    if (shouldGrow(count))
        grow(ml_max(m_size + count, m_cap << 2));
    auto at = const_cast<iterator>(pos);
    if (std::distance(at + count, end()) < 0)
    {   
        for (auto itr = at; itr != end(); itr++)
            std::swap(*itr, *(itr + count));
    }
    else
    {
        for (auto itr = end() - 1; itr >= at; itr--)
            std::swap(*itr, *(itr + count));
    }
    return at;
}
#endif
} // namespace mylib