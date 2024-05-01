#include <containers/growing_array.h>
#include <containers/arena.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>

#define Print(arr) _print(#arr, arr)

// Is not thread-safe
template<class Object>
class GrowingArray {
public:
    using iterator = Object*;
    using const_iterator = const Object*;

    GrowingArray(ml::MemoryArena* arena)
    : m_arena(arena), m_chunk(nullptr){}
    ~GrowingArray() {
        m_arena->release_memory_chunk(m_chunk);
    }

    void push_back(const Object& obj) {
        uint64_t obj_size = sizeof(obj);
        if (m_chunk == nullptr) {
            m_chunk = m_arena->get_memory_chunk(nullptr, obj_size);
        } else if (m_chunk->left() < obj_size) {
            m_chunk = m_arena->get_memory_chunk(m_chunk, m_chunk->pos + obj_size);
        }
    }

    void push_back(Object&& obj) {

    }

private:
    ml::MemoryArena *m_arena;
    ml::MemoryChunk *m_chunk;
};

// template<class T> 
// static void _print(const char *name, const GrowingArray<T>& arr) {
//     std::cout << name << ": {";
//     for (auto itr = arr.begin(); itr != arr.end(); itr++) {
//         if (itr == arr.end() - 1) {
//             std::cout << *itr;
//             break;
//         }
//         std::cout << *itr << ", ";
//     }
//     std::cout << "};" << std::endl;
// }

TEST(Arena, Creation) {
    ml::MemoryArena arena;

    GrowingArray<int32_t> arr(&arena);
}