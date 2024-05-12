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
class invalid_iterator : public std::exception {
public: 
    invalid_iterator() noexcept {}

    explicit invalid_iterator(const char *msg) noexcept 
    : std::exception(msg) {}
};
} // namespace detail

template<class Object>
class GrowingArray
{
public:
    /**
     * 
    */
    class const_iterator {
    public:
        const_iterator() : m_object(nullptr), m_chunkp(nullptr) {}
        
        const Object& operator*() const { return static_cast<const Object&>(*m_object); }

        const_iterator& operator++() {
            if (*m_chunkp != m_old_chunk) {
                throw detail::invalid_iterator();
            }
            
            m_object++; 
            return *this; 
        }
        const_iterator operator++(int /*postfix*/) { const_iterator old_this = *this; ++(*this); return old_this; }
        const_iterator operator--()                { isValid(); m_object--; return *this; }
        const_iterator operator--(int /*postfix*/) { const_iterator old_this = *this; --(*this); return old_this; }

        bool operator==(const const_iterator& rhs) const { return (m_object == rhs.m_object); }
        bool operator!=(const const_iterator& rhs) const { return !(*this == rhs); }

    protected:
        void isValid() const {
            if (m_object == nullptr) throw std::out_of_range("iterator is nullptr"); 
        }

        const_iterator(Object* object, Chunk** chunkp) 
        : m_object(object), m_chunkp(chunkp), m_old_chunk(*chunkp) {}

        Object* m_object;
        Chunk** m_chunkp;
        Chunk* m_old_chunk;
        friend class GrowingArray<Object>;
    };

    class iterator : public const_iterator {
    public:
        iterator() {}

        Object& operator*()             { return const_cast<Object&>(const_iterator::operator*()); }
        const Object& operator*() const { return const_iterator::operator*(); }

        iterator& operator++()               { const_iterator::m_object++; return *this; }
        iterator operator++(int /*postfix*/) { iterator old_this = *this; ++(*this); return old_this; }
        iterator& operator--()               { const_iterator::isValid(); const_iterator::m_object--; return *this; }
        iterator operator--(int /*postfix*/) { const_iterator::isValid(); iterator old_this = *this; --(*this); return old_this; }

    protected:
        friend class GrowingArray<Object>;
        iterator(Object* object) : const_iterator(object) {}
    };

    iterator begin() { 
        return (m_chunk != 0) ? iterator(reinterpret_cast<Object*>(m_chunk->begin(), &m_chunk)) : iterator(); 
    }

    iterator end() { 
        return (m_chunk != 0) ? iterator(reinterpret_cast<Object*>(m_chunk->end(), &m_chunk)) : iterator(); 
    }

    const_iterator begin() const { return (m_chunk != 0) ? const_iterator(reinterpret_cast<Object*>(m_chunk->begin())) : const_iterator(); }
    const_iterator end() const { return (m_chunk != 0) ? const_iterator(reinterpret_cast<Object*>(m_chunk->end())) : const_iterator(); }

    // TODO: Reserve memory chunk in the beginning.
    explicit GrowingArray(Arena* arena)
     : m_arena(arena), m_chunk(nullptr), m_size(0) {}

    ~GrowingArray() {
        if (m_chunk)
            m_arena->releaseChunk(m_chunk);
    }

    GrowingArray(const GrowingArray<Object>& rhs) {
        m_arena = rhs.m_arena;
        if (rhs.m_chunk) {
            m_chunk = rhs.m_arena->getChunk(rhs.m_chunk->size());
            m_chunk->copy(rhs.m_chunk);
        }
        m_size = rhs.m_size;
    }

    GrowingArray& operator=(const GrowingArray<Object>& rhs) {
        if (m_chunk) 
            m_arena->releaseChunk(m_chunk);
        if (rhs.m_chunk) {
            m_chunk = m_arena->getChunk(rhs.m_chunk->size());
            m_chunk->copy(rhs.m_chunk);
        }
        m_size = rhs.m_size;
        return *this;
    }

    GrowingArray(GrowingArray&& rhs) {
        m_arena = rhs.m_arena;
        m_chunk = rhs.m_chunk;
        m_size = rhs.m_size;
        rhs.m_chunk = nullptr;
    }

    GrowingArray& operator=(GrowingArray&& rhs) {
        // NOTE: This is exactly the code we have in copy assignment operator,
        // so we can eliminate that.
        if (m_chunk)
            m_arena->releaseChunk(m_chunk);
        if (rhs.m_chunk) {
            m_chunk = m_arena->getChunk(rhs.m_chunk->size());
            m_chunk->copy(rhs.m_chunk);
            rhs.m_chunk = nullptr;
        }
        m_size = rhs.m_size;
        return *this;
    }

    const Object& operator[](int idx) const { 
        // pull out into a separate function.
        if (idx < 0 || idx >= m_size) 
            throw std::out_of_range(fmt::format("index {} is out of range", idx));
    }

    Object& operator[](int idx) { 
        if (idx < 0 || idx >= m_size) 
            throw std::out_of_range(fmt::format("index {} is out of range", idx));
    }

    void push_back(const Object& obj) {
        // resize changes the size to fit (N) elements and copies the previous elements.
        std::uint64_t obj_size = sizeof(obj); // this should be computed inside resize, reserve
        if (!m_chunk)
            m_chunk = m_arena->getChunk(obj_size);
        else if (obj_size > m_chunk->remainingSpace())
            m_chunk = m_arena->getChunk(m_chunk->size()*2, m_chunk);
        m_chunk->push(obj);
        m_size += 1;
    }

    void push_back(Object&& obj) {
        std::uint64_t obj_size = sizeof(obj);
        if (!m_chunk)
            m_chunk = m_arena->getChunk(obj_size);
        else if (obj_size > m_chunk->remainingSpace())
            m_chunk = m_arena->getChunk(m_chunk->size()*2, m_chunk);
        m_chunk->push(std::move(obj));
        m_size += 1;
    }

    const Object& back() const {

    }

    const Object& front() const {

    }

    void pop_back() {

    }

    std::size_t size() const { return m_size; } 

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

    template<class ...Args>
    iterator emplace(const_iterator pos, Args&&...args)
    {
        return insert(pos, Object(std::forward<Args>(args)...));
    } 

    // TODO: Implement
    void flush(GrowingArray& other);
#endif

    // void clear();
    // size_t size() const { return m_size; }
    // size_t capacity() const { return m_cap; }
    // bool empty() const { return m_size == 0; }
    // size_t left() const { return m_cap - m_size; }

private:
    Arena*      m_arena;
    Chunk*      m_chunk;
    std::size_t m_size;
    std::size_t m_cap; // Has to be computed based on (chunk->size() / sizeof(Object)) 
};

#if 0
/*************************************************************************
* Experimenting with const_iterator and iterator begin classes rather than pointers.
* Once the implementation is stable, all iterators inside GrowingArray will be replaced 
* with custom ConstIterator and Iterator classes.
* But for now they are not in use anywhere!!!
*************************************************************************/
template<class Object>
class GrowingArray<Object>::ConstIterator
{
public:
    ConstIterator operator++()
    {
        assert(m_at + 1 <= end());
        ++m_at;
        return *this;
    }

    ConstIterator operator++(int unused_)
    {
        assert(m_at + 1 <= end());
        auto res = *this;
        ++(*this);
        return res;
    }

    // ConstIterator operat[]

    const Object& operator*()
    {
        assert(m_at >= begin() && m_at < end()); 
        return *m_at;
    }

protected:
    Object *m_at;
    // TODO: Maintain a flag
    // bool is_valid;
    // friend class GrowingArray<Object>;
};

template<class Object>
class GrowingArray<Object>::Iterator : public ConstIterator
{
public:
};

/*************************************************************************/

template<class Object>
GrowingArray<Object>::GrowingArray()
{}

template<class Object>
GrowingArray<Object>::GrowingArray(size_t cap)
: m_cap(cap) 
{ m_data = alloc_memory(cap); }

template<class Object>
GrowingArray<Object>::GrowingArray(std::initializer_list<Object> list)
{
    for (const auto& item : list)
        push_back(item);
}

template<class Object>
GrowingArray<Object>::~GrowingArray()
{ free_memory(m_data); }

template<class Object>
GrowingArray<Object>::GrowingArray(const GrowingArray<Object>& rhs)
: m_size(rhs.m_size),
m_cap(rhs.m_cap)
{
    m_data = alloc_memory(rhs.m_cap);
    memcpy(m_data, rhs.m_data, rhs.m_size*sizeof(Object));
}

template<class Object>
GrowingArray<Object>& GrowingArray<Object>::operator=(const GrowingArray<Object>& rhs) {
    if (this == &rhs) return *this;
    Object* data = alloc_memory(rhs.m_cap);
    memcpy(data, rhs.m_data, rhs.m_size*sizeof(Object));
    Object* tmp = m_data;
    m_size = rhs.m_size;
    m_cap = rhs.m_cap;
    m_data = data;
    free_memory(tmp);
    return *this;
}

template<class Object>
void GrowingArray<Object>::zeroMembers() {
    m_size = m_cap = 0; 
    m_data = nullptr;
}

template<class Object>
GrowingArray<Object>::GrowingArray(GrowingArray&& rhs)
: m_size(rhs.m_size), 
m_cap(rhs.m_cap),
m_data(rhs.m_data)
{ rhs.zeroMembers(); }

template<class Object>
GrowingArray<Object>& GrowingArray<Object>::operator=(GrowingArray&& rhs) {
    if (this == &rhs) return *this;
    Object* tmp = m_data;
    m_size = rhs.m_size;
    m_cap = rhs.m_cap;
    m_data = rhs.m_data;
    rhs.zeroMembers();
    free_memory(tmp);
    return *this;
}

template<class Object>
Object* GrowingArray<Object>::alloc_memory(size_t count) {
    size_t alloc_size = count*sizeof(Object);
#ifdef _WIN32
    void *memory = VirtualAlloc(0, alloc_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#else
    void* memory = malloc(alloc_size);
    memset(memory, 0, alloc_size);
#endif
    return static_cast<Object*>(memory);
}

template<class Object>
void GrowingArray<Object>::free_memory(Object *memory) {
#ifdef _WIN32
    VirtualFree((LPVOID)memory, 0, MEM_RELEASE);
#else
    free(memory);  
#endif 
    zeroMembers(); 
}

template<class Object>
void GrowingArray<Object>::push_back(const Object& obj) {
    if (shouldGrow(1)) {
        auto cap = ml_max(16, m_cap << 2); 
        grow(cap);
    }
    m_data[m_size++] = obj;
}

template<class Object>
void GrowingArray<Object>::push_back(Object&& obj) {
    if (shouldGrow(1)) {
        auto cap = ml_max(16, m_cap << 2);
        grow(cap);
    }
    m_data[m_size++] = std::move(obj);
}

template<class Object>
const Object& GrowingArray<Object>::back() const {
    assert(m_size > 0);
    return m_data[m_size - 1];
}

template<class Object>
const Object& GrowingArray<Object>::front() const {
    assert(m_size > 0);
    return m_data[0];
}

template<class Object>
void GrowingArray<Object>::pop_back() {
    assert(m_size > 0);
    Object* start = (m_data + m_size - 1); 
    memset(start, 0, sizeof(Object));
    m_size--;
}

template<class Object>
bool GrowingArray<Object>::grow(size_t cap)
{
    if (cap <= m_cap)
        return false;
    for(; (cap & (cap - 1)) != 0; cap++); 
    Object* data = alloc_memory(cap);
    if (m_data != nullptr)
        memcpy(data, m_data, m_size*sizeof(Object));
    free_memory(m_data);
    m_data = data;
    m_cap = cap;
    return true;
}

template<class Object>
void GrowingArray<Object>::clear()
{
    if (m_size > 0)
        memset(m_data, 0, m_size*sizeof(Object));
    m_size = 0;
}

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