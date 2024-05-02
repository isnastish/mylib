#include <containers/growing_array.h>
#include <containers/arena.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

#define print(arr) _print(#arr, arr)

class ArenaFixture : public ::testing::Test {
protected:
    constexpr static uint64_t small_arena_size{64};
    constexpr static uint64_t medium_arena_size{1024*1024};
    constexpr static uint64_t large_arena_size{1024*1024*1024};
};

// template<class Object> class GrowingArray;

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

// template<class Object>
// class GrowingArray {
// public:
//     using iterator = Object*;
//     using const_iterator = const Object*;

//     GrowingArray(ml::MemoryArena* arena)
//     : m_arena(arena), m_chunk(nullptr), m_size(0) {}
//     ~GrowingArray() {
//         m_arena->release_memory_chunk(m_chunk);
//     }

//     void push_back(const Object& obj) {
//         uint64_t obj_size = sizeof(obj);
//         if ((m_chunk == nullptr) || (m_chunk->remaining() < obj_size)) {
//             m_chunk = m_arena->get_memory_chunk(m_chunk, obj_size);
//         }
//         m_chunk->push(obj, obj_size);
//         m_size++;
//     }

//     void push_back(Object&& obj) {
//         uint64_t obj_size = sizeof(obj);
//         if ((m_chunk == nullptr) || (m_chunk->remaining() < obj_size)) {
//             m_chunk = m_arena->get_memory_chunk(m_chunk, obj_size);
//         }
//         m_chunk->push(std::move(obj), obj_size);
//         m_size++;
//     }

//     void pop_back() {
//         assert(size() != 0);
//         m_chunk->pop();
//         m_size--;
//     }

//     size_t size() const { return m_size; }

//     iterator begin() { 
//         return (iterator)m_chunk->begin();
//     }

//     iterator end() {
//         return (iterator)m_chunk->end();
//     }

//     const_iterator begin() const {
//         return (const_iterator)(m_chunk->begin());
//     }

//     const_iterator end() const {
//         return (const_iterator)(m_chunk->end());
//     }

// private:
//     ml::MemoryArena *m_arena;
//     ml::MemoryChunk *m_chunk;
//     size_t m_size;
// };

// This would be an integration test
// TEST(Arena, Creation) {
//     constexpr size_t COUNT = 1024*1024;
//     ml::MemoryArena arena;
//     GrowingArray<int32_t> arr(&arena);
//     for (int i = 0; i < COUNT; i++) {
//         arr.push_back(i); //(i + 1) << 2);
//     }
//     ASSERT_EQ(arr.size(), COUNT);
//     print(arr);
// }

TEST(Arena, ForceAlignSize) {
    ml::MemoryArena arena(64);
    ASSERT_EQ(arena.capacity(), ml::MemoryArena::PAGE_SIZE);
}

TEST(Arena, SizeOfTwoPages) {
    ml::MemoryArena arena(1033);
    ASSERT_EQ(arena.capacity(), ml::MemoryArena::PAGE_SIZE*2);
}

TEST(Arena, CreateWithDefaultSize) {
    ml::MemoryArena arena;
    ASSERT_EQ(arena.capacity(), ml::MemoryArena::ALLOC_SIZE);
}

TEST(Arena, GetSingleChunk) {
    constexpr uint64_t size = 1024*1024;
    ml::MemoryArena arena(size);
    ml::MemoryChunk *chunk = arena.get_memory_chunk(nullptr, 32);
    ASSERT_EQ(arena.remaining(), size - ml::MemoryArena::PAGE_SIZE);
    ASSERT_EQ(arena.total_chunks_count(), 1);
    arena.release_memory_chunk(chunk);
    ASSERT_EQ(arena.empty_chunks_count(), 1);
}

TEST(Arena, GetMultipleChunks) {
    constexpr uint64_t size = 1024*1024;
    ml::MemoryArena arena(size);
    ml::MemoryChunk *chunk_1 = arena.get_memory_chunk(nullptr, 64);
    ASSERT_EQ(arena.total_chunks_count(), 1);
    ml::MemoryChunk *chunk_2 = arena.get_memory_chunk(nullptr, 1025);
    ASSERT_EQ(arena.remaining(), size - ml::MemoryArena::PAGE_SIZE*3);
    ASSERT_EQ(arena.total_chunks_count(), 2);

    arena.release_memory_chunk(chunk_1);
    arena.release_memory_chunk(chunk_2);
    ASSERT_EQ(arena.empty_chunks_count(), 2);
}