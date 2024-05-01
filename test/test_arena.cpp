#include <containers/growing_array.h>
#include <containers/arena.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

#define print(arr) _print(#arr, arr)

template<class Object> class GrowingArray;

template<class T> 
static void _print(const char *name, const GrowingArray<T>& arr) {
    std::cout << name << ": {";
    for (auto itr = arr.begin(); itr != arr.end(); itr++) {
        if (itr == arr.end() - 1) {
            std::cout << *itr;
            break;
        }
        std::cout << *itr << ", ";
    }
    std::cout << "};" << std::endl;
}

template<class Object>
class GrowingArray {
public:
    using iterator = Object*;
    using const_iterator = const Object*;

    GrowingArray(ml::MemoryArena* arena)
    : m_arena(arena), m_chunk(nullptr), m_size(0) {}
    ~GrowingArray() {
        m_arena->release_memory_chunk(m_chunk);
    }

    void push_back(const Object& obj) {
        uint64_t obj_size = sizeof(obj);
        if ((m_chunk == nullptr) || (m_chunk->remaining() < obj_size)) {
            m_chunk = m_arena->get_memory_chunk(m_chunk, obj_size);
        }
        m_chunk->push(obj, obj_size);
        m_size++;
    }

    void push_back(Object&& obj) {
        uint64_t obj_size = sizeof(obj);
        if ((m_chunk == nullptr) || (m_chunk->remaining() < obj_size)) {
            m_chunk = m_arena->get_memory_chunk(m_chunk, obj_size);
        }
        m_chunk->push(std::move(obj), obj_size);
        m_size++;
    }

    void pop_back() {
        assert(size() != 0);
        m_chunk->pop();
        m_size--;
    }

    size_t size() const { return m_size; }

    iterator begin() { 
        return (iterator)m_chunk->begin();
    }

    iterator end() {
        return (iterator)m_chunk->end();
    }

    const_iterator begin() const {
        return (const_iterator)(m_chunk->begin());
    }

    const_iterator end() const {
        return (const_iterator)(m_chunk->end());
    }

    // TODO: Overloaded operator[]

private:
    ml::MemoryArena *m_arena;
    ml::MemoryChunk *m_chunk;
    size_t m_size;
};

TEST(Arena, Creation) {
    constexpr size_t COUNT = 1024*1024;
    ml::MemoryArena arena;
    GrowingArray<int32_t> arr(&arena);
    for (int i = 0; i < COUNT; i++) {
        arr.push_back(i); //(i + 1) << 2);
    }
    ASSERT_EQ(arr.size(), COUNT);
    print(arr);
}