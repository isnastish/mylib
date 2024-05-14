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
    const Object& operator[](int index) const { 
        isValid(index); 
        Object* begin = reinterpret_cast<Object*>(m_chunk->begin());
        return begin[index];
    }

    /**
     * 
    */
    Object& operator[](int index) { 
        isValid(index);
        Object* begin = reinterpret_cast<Object*>(m_chunk->begin()); 
        return begin[index]; 
    }

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
    const Object& back() const { isValid(m_size - 1); Object* data = reinterpret_cast<Object*>(m_chunk->begin()); return data[m_size-1]; }

    /**
     * 
    */
    const Object& front() const { isValid(0); Object* data = reinterpret_cast<Object*>(m_chunk->begin()); return data[0]; }

    /**
     * 
    */
    void pop_back() { isValid(m_size - 1); m_chunk->pop(sizeof(Object)); m_size -= 1; }

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
    const_iterator begin() const noexcept { 
        auto res = getDataRange();
        if (res.has_value()) {
            Range range = res.values();
            return const_iterator(range.first, range.first, range.one_past_last, &m_chunk);
        }
        return const_iterator(); 
    }

    /**
     *
    */
    const_iterator end() const noexcept {
        auto res = getDataRange();
        if (res.has_value()) {
            auto& range = res.values();
            return const_iterator(range.one_past_last, range.first, range.one_past_last, &m_chunk);
        }
        return const_iterator();
    }

    /**
     *
    */
    iterator begin() noexcept {
        auto res = getDataRange();
        if (res.has_value()) {
            auto& range = res.value();
            return iterator(range.first, range.first, range.one_past_last, &m_chunk);
        }
        return iterator();
    }

    /**
     * 
    */
    iterator end() noexcept { 
        auto res = getDataRange();
        if (res.has_value()) {
            auto& range = res.value();
            return iterator(range.one_past_last, range.first, range.one_past_last, &m_chunk);
        }
        return iterator(); 
    }

    /**
     * 
    */
    const_iterator cbegin() const noexcept {
        return begin();
    }

    /**
     * 
    */
    const_iterator cend() const noexcept { 
        return end();
    }   

    /**
     * Insert an element before the position pos. pos can be an end() iterator, 
     * in that case the element will be insert at the end().
     * Only the iterators and references before the insertion point remain valid.
    */
    iterator insert(const_iterator pos, const Object& value) {
        // NOTE: Pos iterator is invalidated.
        const Object* where_ptr = pos.m_object;
        if (where_ptr < m_chunk->begin() || where_ptr > m_chunk->end())
            throw std::out_of_range("iterator is out of range");

        for (auto itr = end(); 
            itr != pos; 
            itr--) {
                *itr = *(itr - 1);
        }

        pos.m_invalid = true;
        *where_ptr = value;
        
        Object* last =  reinterpret_cast<Object*>(m_chunk->end());
        return iterator(where_ptr, last);
    }

    /**
     * 
    */
    iterator insert(const_iterator pos, Object&& value) {
        // NOTE: Pos iterator is invalidated.
        const Object* where_ptr = pos.m_object;
        if (where_ptr < m_chunk->begin() || where_ptr > m_chunk->end())
            throw std::out_of_range("iterator is out of range");

        for (auto itr = end(); 
            itr != pos; 
            itr--) {
                *itr = *(itr - 1);
        }

        pos.m_invalid = true;
        *where_ptr = std::move(value);
        Object* last =  reinterpret_cast<Object*>(m_chunk->end());
        return iterator(where_ptr, last);
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
    struct Range {
        Object* first;
        Object* one_past_last;
    };

    std::optional<Range> getDataRange() {
        if (m_chunk) {
            Object* first = reinterpret_cast<Object*>(m_chunk->begin());
            Object* one_past_last = reinterpret_cast<Object*>(m_chunk->end());
            return {Range{first, one_past_last}};
        }
        return {};
    }

    void isValid(int index) const {
        // If m_chunk is nullptr, the array is empty
        if (m_chunk == nullptr) throw std::out_of_range("");
        // The array is non-empty but an index is out of range
        if (index < 0 || index >= m_size) throw std::out_of_range(fmt::format("index {} is out of range", index));
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
    enum class Op;
public:
    const_iterator() noexcept 
    {}

    const_iterator operator+(int count) const {
        isValid(Op::INCREMENT, count);
        return const_iterator(m_cur + count);
    }   

    const_iterator operator-(int count) const {
        isValid(Op::DECREMENT, count);
        return const_iterator(m_cur - count);
    }

    const_iterator& operator+=(int count) {
        isValid(Op::INCREMENT, count);
        m_cur += count;
        return *this;
    }
    
    const_iterator& operator-=(int count) {
        // count can be 0
        isValid(Op::DECREMENT, count);
        m_cur -= count;
        return *this;
    }

    const Object& operator*() const {
        isValid();
        return static_cast<const Object&>(*m_cur); 
    }

    const_iterator& operator++() {
        isValid();
        m_cur++;
        return *this;
    }

    const_iterator operator++(int /*postfix*/) { 
        const_iterator old_this = *this; 
        ++(*this);
        return old_this; 
    }

    const_iterator operator--() { 
        isValid(Op::DECREMENT);
        m_cur--; 
        return *this; 
    }

    const_iterator operator--(int /*postfix*/) { 
        const_iterator old_this = *this;
        --(*this);
        return old_this; 
    }

    bool operator==(const const_iterator& rhs) const noexcept {
        // Iterators might point to an empty collection
        return (m_cur == rhs.m_cur); 
    }

    bool operator!=(const const_iterator& rhs) const noexcept { 
        // Iterators might point to an empty collection
        return !(*this == rhs); 
    }

protected:
    enum class Op : std::int32_t {
        INCREMENT,
        DECREMENT,
    };

    friend class GrowingArray<Object>;

    void isValid(Op op=Op::INCREMENT, int count=1) const {
        // The collection is empty.
        if (m_cur == nullptr) throw std::out_of_range("");
        // Iterator was marked as invalid after erase or insert call.
        if (m_invalid) throw detail::invalid_iterator_error("");
        // Memory was reallocated, and thus all iterators became invalid
        if (*m_chunk_ptr != m_old_chunk) throw detail::invalid_iterator_error("");
        // Cannot increment end() iterator, but can be equal to end() iterator.
        if ((op == Op::INCREMENT) && ((m_cur + count) > m_one_past_last)) throw std::out_of_range("");
        // Cannot decrement begin() iterator, but can be equal to begin() iterator.
        if ((op == Op::DECREMENT) && ((m_cur - count) < m_first)) throw std::out_of_range("");
    }

    const_iterator(Object* cur, Object* first, Object* last, Chunk** chunk_ptr)
    : m_cur(cur), 
    m_first(first), 
    m_one_past_last(last), 
    m_chunk_ptr(chunk_ptr), 
    m_old_chunk(*chunk_ptr)
    {}

    bool    m_invalid       = false;
    Object* m_cur           = nullptr;
    Object* m_first         = nullptr;
    Object* m_one_past_last = nullptr;
    Chunk** m_chunk_ptr     = nullptr;
    Chunk* m_old_chunk      = nullptr;
};

template<class Object>
class GrowingArray<Object>::iterator : public const_iterator { // is-a
public:
    iterator() noexcept {}

    Object& operator*() { 
        return const_cast<Object&>(const_iterator::operator*()); 
    }

    const Object& operator*() const { 
        return const_iterator::operator*(); 
    }

    iterator& operator++() {
        const_iterator::isValid();
        const_iterator::m_cur++; 
        return *this; 
    }

    iterator operator++(int /*postfix*/) { 
        iterator old_this = *this; 
        ++(*this); 
        return old_this; 
    }

    iterator& operator--() { 
        const_iterator::isValid(); 
        const_iterator::m_cur--; 
        return *this; 
    }

    iterator operator--(int /*postfix*/) { 
        iterator old_this = *this; 
        --(*this); 
        return old_this; 
    }
    
    iterator operator+(int count) const {
        const_iterator::isValid(const_iterator::Op::INCREMENT, count);
        return iterator(
            const_iterator::m_cur + count, 
            const_iterator::m_first, 
            const_iterator::m_one_past_last,
            const_iterator::m_chunk_ptr); 
    }

    iterator operator-(int count) const {
        const_iterator::isValid(const_iterator::Op::DECREMENT, count);
        return iterator(
            const_iterator::m_cur + count,
            const_iterator::m_first,
            const_iterator::m_one_past_last,
            const_iterator::m_chunk_ptr);
    }

    iterator& operator+=(int count) {
        const_iterator::isValid(const_iterator::Op::INCREMENT, count);
        const_iterator::m_cur += count;
        return *this;
    }

    iterator& operator-=(int count) {
        const_iterator::isValid(const_iterator::Op::DECREMENT, count);
        const_iterator::m_cur -= count;
        return *this;
    }

protected:
    friend class GrowingArray<Object>;

    iterator(Object* cur, Object* first, Object* one_past_last, Chunk** chunk_ptr) noexcept  
    : const_iterator(cur, first, one_past_last, chunk_ptr) {}
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