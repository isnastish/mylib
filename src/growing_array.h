#pragma once

#include <initializer_list>
#include <utility> // std::move
#include <algorithm> // std::max
#include <cassert>

#define ml_max(a, b) ((a) > (b) ? (a) : (b))

namespace ml
{
template<class Object>
class GrowingArray
{
public:
    using iterator = Object*;
    using const_iterator = const Object*;

    explicit GrowingArray(size_t cap=16);
    GrowingArray(std::initializer_list<Object> list);
    ~GrowingArray();

    GrowingArray(const GrowingArray<Object>& rhs);
    GrowingArray& operator=(const GrowingArray<Object>& rhs);
    GrowingArray(GrowingArray&& rhs);
    GrowingArray& operator=(GrowingArray&& rhs);

    void push_back(const Object& obj);
    void push_back(Object&& obj);
    const Object& back() const;
    const Object& front() const;
    void pop_back();
    bool grow(size_t cap);
    
    iterator erase(iterator place);
    iterator erase(iterator first, iterator second);
    iterator erase(const_iterator place);
    iterator erase(const_iterator first, const_iterator second);

    iterator insert(iterator place, const Object& obj);
    iterator insert(iterator place, Object&& obj);
    iterator insert(iterator place, size_t count, const Object& obj);
    iterator insert_range(iterator first, iterator last);
    
    void flush(GrowingArray& other);

    void clear();
    size_t size() const { return m_size; }
    size_t capacity() const { return m_cap; }
    bool empty() const { return m_size == 0; }
    size_t left() const { return m_cap - m_size; }

    iterator begin() { return m_data; }
    iterator end() { return m_data + m_size; }

    const_iterator begin() const { return m_data; }
    const_iterator end() const { return m_data + m_size; }

private:
    Object* alloc_memory(size_t count);
    void free_memory(Object *memory);
    void zero();

    Object* m_data{nullptr};
    size_t m_size{0};
    size_t m_cap{0};
};

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
{ delete []m_data; }

template<class Object>
GrowingArray<Object>::GrowingArray(const GrowingArray<Object>& rhs)
: m_size(rhs.m_size),
m_cap(rhs.m_cap)
{
    m_data = alloc_memory(rhs.m_cap);
    memcpy(m_data, rhs.m_data, rhs.m_size*sizeof(Object));
}

template<class Object>
GrowingArray<Object>& GrowingArray<Object>::operator=(const GrowingArray<Object>& rhs)
{
    if (this == &rhs)
        return *this;
    Object* data = alloc_memory(rhs.m_cap);
    memcpy(data, rhs.m_data, rhs.m_size*sizeof(Object));
    free_memory(m_data);
    m_size = rhs.m_size;
    m_cap = rhs.m_cap;
    m_data = data;
    return *this;
}

template<class Object>
void GrowingArray<Object>::zero()
{
    m_size = 0; 
    m_cap = 0; 
    m_data = nullptr;
}

template<class Object>
GrowingArray<Object>::GrowingArray(GrowingArray&& rhs)
: m_size(rhs.m_size), 
m_cap(rhs.m_cap),
m_data(rhs.m_data)
{ rhs.zero(); }

template<class Object>
GrowingArray<Object>& GrowingArray<Object>::operator=(GrowingArray&& rhs)
{
    if (this == &rhs)
        return *this;
    free_memory(m_data);
    m_size = rhs.m_size;
    m_cap = rhs.m_cap;
    m_data = rhs.m_data;
    rhs.zero();
    return *this;
}

template<class Object>
Object* GrowingArray<Object>::alloc_memory(size_t count)
{
    size_t alloc_size = count*sizeof(Object);
    void* memory = malloc(alloc_size);
    memset(memory, 0, alloc_size);
    return static_cast<Object*>(memory);
}

template<class Object>
void GrowingArray<Object>::free_memory(Object *memory)
{ free(memory); zero(); }

template<class Object>
void GrowingArray<Object>::push_back(const Object& obj)
{
    if (m_size == m_cap)
    {
        // TODO: Figure out the problem with std::max
        auto cap = ml_max(16, m_cap << 2); 
        grow(cap);
    }
    m_data[m_size++] = obj;
}

template<class Object>
void GrowingArray<Object>::push_back(Object&& obj)
{
    if (m_size == m_cap)
    {
        auto cap = ml_max(16, m_cap << 2);
        grow(m_cap << 2);
    }
    m_data[m_size++] = std::move(obj);
}

template<class Object>
const Object& GrowingArray<Object>::back() const
{
    assert(m_size > 0);
    return static_cast<const Object&>(*(m_data + m_size - 1));
}

template<class Object>
const Object& GrowingArray<Object>::front() const
{
    assert(m_size > 0);
    return static_cast<const Object&>(m_data[0]);
}

template<class Object>
void GrowingArray<Object>::pop_back()
{
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
    Object* data = alloc_memory(cap);
    if (m_data != nullptr)
        memcpy(data, m_data, m_size*sizeof(Object));
    free(m_data);
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

// end() iterator cannot be used
// returns the element following the one being removed
template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::erase(iterator place) 
{
    assert(place >= begin() && place < end());
    auto next = place;
    for (; next != (end() - 1); next++)
        *next = *(next + 1);
    // memset(next, 0, sizeof(Object));
    m_size--;
    return place;
}

// erase all the elements starting from first up to last, but not inclusively
template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::erase(iterator first, iterator last)
{
    // TODO: Zero the memory
    assert((first >= begin() && first <= end()) &&
    (last >= begin() && last <= end()));
    size_t erase_size = last - first;
    for (auto at = last; at != end();)
        *first++ = *at++;
    m_size -= erase_size;
    return first;
}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::erase(const_iterator place)
{

}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::erase(const_iterator first, const_iterator second)
{

}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::insert(iterator place, const Object& obj)
{

}

// return 
template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::insert(iterator place, Object&& obj)
{

}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::insert(iterator place, size_t count, const Object& obj)
{

}

template<class Object>
GrowingArray<Object>::iterator GrowingArray<Object>::insert_range(iterator first, iterator last)
{

}

} // namespace ml