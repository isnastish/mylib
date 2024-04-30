#include <containers/growing_array.h>
#include <containers/arena.h>
#include <gtest/gtest.h>
#include <iostream>

#define Print(arr) _print(#arr, arr)

template<class T>
class GrowingArray {
public:
    using iterator = T*;
    using const_iterator = const T*;

    GrowingArray(ml::MemoryArena *arena)
    : m_arena(arena), 
    m_data(reinterpret_cast<T*>(m_arena->m_ptr + m_arena->m_pos)), 
    m_size(0) {
        
    }

    void push_back(const T& obj) {
        m_arena->push(obj);
        m_size++;
    }

    void push_back(T&& obj) {
        m_arena->push(std::move(obj));
        m_size++;
    }

    void pop_back() const {
        m_arena->pop();
        m_size--;
    }

    iterator begin() { return &m_data[0]; }
    iterator end() { return &m_data[m_size]; }

    const_iterator begin() const { return &m_data[0]; }
    const_iterator end() const { return &m_data[m_size]; }

private:
    ml::ArenaList *m_arena_list;
    ml::MemoryArena *m_arena; // don't need this
    T *m_data;
    size_t m_size;
};

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

TEST(Arena, Creation) {
    ml::MemoryArena arena;
    GrowingArray<int32_t> arr(&arena);

    arr.push_back(2344);
    arr.push_back(344);
    arr.push_back(-2344);

    int32_t first = *arr.begin();
    // ASSERT_EQ(first, 2344);
    Print(arr);
}