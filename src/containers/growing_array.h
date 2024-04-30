// TODO:
// [X] Replace malloc with VirtualAlloc or HeapAlloc 
// [x] Implement insert functions.
// [x] emplace
// [x] Overloaded operator[]
// [x] Test erasing the elements using const_iterator 
// [ ] const_iterator and iterator as a class
// [ ] Figure out whether we can dereference end() iterator inside std::vector
// [ ] Use custom Arena and pass it to array's ctor.

#pragma once

#include "arena.h"
#include <initializer_list>
#include <utility> // std::move
#include <algorithm> // std::max
#include <cassert>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#define ml_max(a, b) ((a) > (b) ? (a) : (b))

namespace ml
{
template<class Object>
class GrowingArray
{
public:
    class ConstIterator;
    class Iterator;

    using iterator = Object*;
    using const_iterator = const Object*;

    GrowingArray();
    explicit GrowingArray(size_t cap);
    GrowingArray(std::initializer_list<Object> list);
    ~GrowingArray();

    GrowingArray(const GrowingArray<Object>& rhs);
    GrowingArray& operator=(const GrowingArray<Object>& rhs);
    GrowingArray(GrowingArray&& rhs);
    GrowingArray& operator=(GrowingArray&& rhs);

    const Object& operator[](int idx) const { assert(idx >= 0 && idx < size()); return m_data[idx]; }
    Object& operator[](int idx) { assert(idx >= 0 && idx < size()); return m_data[idx]; }

    void push_back(const Object& obj);
    void push_back(Object&& obj);
    const Object& back() const;
    const Object& front() const;
    void pop_back();
    bool grow(size_t cap);
    
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

    void clear();
    size_t size() const { return m_size; }
    size_t capacity() const { return m_cap; }
    bool empty() const { return m_size == 0; }
    size_t left() const { return m_cap - m_size; }

    iterator begin() { return &m_data[0]; }
    iterator end() { return &m_data[size()]; }

    const_iterator begin() const { return &m_data[0]; }
    const_iterator end() const { return &m_data[size()]; }

private:
    iterator shift(const_iterator pos, size_t count);
    Object* alloc_memory(size_t count);
    void free_memory(Object *memory);
    void zeroMembers();
    bool shouldGrow(size_t count) const { return (m_size + count > m_cap); } 

    Object* m_data{nullptr};
    size_t m_size{0};
    size_t m_cap{0};
    MemoryArena *m_arena;
};

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

} // namespace ml