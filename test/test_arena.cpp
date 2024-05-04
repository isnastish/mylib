#include <containers/growing_array.h>
#include <containers/arena.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <array>

class ArenaFixture : public ::testing::Test {
protected:
    constexpr static uint64_t m_small_arena_size{1024*10};
    constexpr static uint64_t m_medium_arena_size{1024*1024};
    constexpr static uint64_t m_large_arena_size{1024*1024*1024};
    constexpr static uint64_t m_chunk_header_size{sizeof(mylib::MemoryChunk)};
};

using ArenaDeathFixture = ArenaFixture;

TEST(Arena, ForceAlignSize) {
    mylib::MemoryArena arena(64);
    ASSERT_EQ(arena.capacity(), mylib::MemoryArena::PAGE_SIZE);
}

TEST(Arena, SizeOfTwoPages) {
    mylib::MemoryArena arena(1033);
    ASSERT_EQ(arena.capacity(), mylib::MemoryArena::PAGE_SIZE*2);
}

TEST(Arena, CreateWithDefaultSize) {
    mylib::MemoryArena arena;
    ASSERT_EQ(arena.capacity(), mylib::MemoryArena::ALLOC_SIZE);
}

TEST_F(ArenaFixture, GetSingleChunk) {
    mylib::MemoryArena arena(m_medium_arena_size);
    mylib::MemoryChunk *chunk = arena.get_memory_chunk(32);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - mylib::MemoryArena::PAGE_SIZE);
    ASSERT_EQ(arena.total_chunks_count(), 1);
    arena.release_memory_chunk(chunk);
    ASSERT_EQ(arena.empty_chunks_count(), 1);
}

TEST_F(ArenaFixture, GetMultipleChunks) {
    mylib::MemoryArena arena(m_medium_arena_size);
    mylib::MemoryChunk *chunk_1 = arena.get_memory_chunk(64);
    ASSERT_EQ(arena.total_chunks_count(), 1);
    mylib::MemoryChunk *chunk_2 = arena.get_memory_chunk(1025);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - mylib::MemoryArena::PAGE_SIZE*3);
    ASSERT_EQ(arena.total_chunks_count(), 2);

    arena.release_memory_chunk(chunk_1);
    arena.release_memory_chunk(chunk_2);
    ASSERT_EQ(arena.empty_chunks_count(), 2);
}

TEST_F(ArenaFixture, GetFreeChunk) {
    constexpr uint64_t chunk_1_size = mylib::MemoryArena::PAGE_SIZE*5;
    mylib::MemoryArena arena(m_medium_arena_size);
    auto *chunk_1 = arena.get_memory_chunk(chunk_1_size);
    //  +----------------------------------------------------------------------------+
    //  |              |      |                 empty space                          |
    //  |              |      |                 empty space                          |
    //  +----------------------------------------------------------------------------+
    //  |              |
    //  |           chunk2(1000) + 24
    //  |
    // chunk1(6120) + 24 <- this chunk will be released first, put into `Free` state,
    // and then reallocated. 
    ASSERT_EQ(arena.remaining(), (m_medium_arena_size - (chunk_1_size + mylib::MemoryArena::PAGE_SIZE)));
    auto *chunk_2 = arena.get_memory_chunk(1000);
    ASSERT_EQ(arena.remaining(), (m_medium_arena_size - (chunk_1_size + 2*mylib::MemoryArena::PAGE_SIZE)));
    arena.release_memory_chunk(chunk_1);
    ASSERT_EQ(arena.empty_chunks_count(), 1);
    chunk_1 = arena.get_memory_chunk(chunk_1_size);
    ASSERT_EQ(arena.total_chunks_count(), 2);
    ASSERT_EQ(arena.empty_chunks_count(), 0);
    arena.release_memory_chunk(chunk_1);
    arena.release_memory_chunk(chunk_2);
}

TEST_F(ArenaFixture, ExtendChunk) {
    constexpr uint64_t chunk_size = 1024;
    mylib::MemoryArena arena(m_medium_arena_size);
    auto *chunk = arena.get_memory_chunk(chunk_size);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - (2*mylib::MemoryArena::PAGE_SIZE));
    // extend the current chunk
    chunk = arena.get_memory_chunk(chunk_size, chunk);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - (3*mylib::MemoryArena::PAGE_SIZE)); 
    ASSERT_EQ(chunk->size(), (chunk_size*3) - m_chunk_header_size);
    chunk = arena.get_memory_chunk(64, chunk);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - (4*mylib::MemoryArena::PAGE_SIZE));
    ASSERT_EQ(chunk->size(), (chunk_size*4) - m_chunk_header_size);
}

TEST_F(ArenaFixture, FindOptimalFreeChunk) {
    constexpr uint64_t chunk_size = 1024;
    mylib::MemoryArena arena(m_medium_arena_size);
    std::vector<mylib::MemoryChunk*> chunks;
    constexpr int chunks_count = 2;
    chunks.reserve(chunks_count);
    uint64_t total_size = 0;
    for (int i = 1; i <= chunks_count; i++) {
        uint64_t alloc_size = chunk_size*i; 
        total_size += (alloc_size) + (mylib::MemoryArena::PAGE_SIZE - (alloc_size % mylib::MemoryArena::PAGE_SIZE));
        chunks.push_back(arena.get_memory_chunk(alloc_size));
    }
    std::cout << "total_size: " << total_size << std::endl;
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - total_size);
    ASSERT_EQ(arena.total_chunks_count(), 2);
    ASSERT_EQ(arena.empty_chunks_count(), 0);
    ASSERT_EQ(chunks[0]->size(), 2024);
    ASSERT_EQ(chunks[1]->size(), 3048);

    //  +----------------------------------------------------------------------------+
    //  |     |                  |             |          empty space                |
    //  |     |                  |             |          empty space                |
    //  +----------------------------------------------------------------------------+
    //  |     |                  |
    //  |   chunk2(3048)       chunk3(1024) <- this chunk would be released (and then reassigned as a new chunk, as the most optimal)
    //  |
    // chunk1(2024) <- this chunk would be released

    constexpr uint64_t small_chunk_size = 64;
    chunks.push_back(arena.get_memory_chunk(small_chunk_size));
    ASSERT_EQ(arena.total_chunks_count(), 3);
    chunks.erase(std::remove_if(chunks.begin(), chunks.end(), [&arena](mylib::MemoryChunk* chunk) {
        if (chunk->size() < (2*mylib::MemoryArena::PAGE_SIZE)) {
            arena.release_memory_chunk(chunk);
            return true;
        }
        return false;
    }), chunks.end());
    ASSERT_EQ(arena.empty_chunks_count(), 2);
    ASSERT_EQ(arena.total_chunks_count(), 3);
    // Skip the first empty chunk and find the most optimal (with the size closest to what we want).
    auto *new_chunk = arena.get_memory_chunk(small_chunk_size);
    ASSERT_EQ(new_chunk->size(), 1000);
}

// TODO: These tests have to be removed once we add a functionality that the arena can grow.
TEST_F(ArenaDeathFixture, RunOutOfSpaceWhenExtendingTheChunk) {
    constexpr uint64_t chunks_size = (m_medium_arena_size - (2*mylib::MemoryArena::PAGE_SIZE));
    mylib::MemoryArena arena(m_medium_arena_size);
    auto *chunk = arena.get_memory_chunk(chunks_size);
    ASSERT_EQ(arena.total_chunks_count(), 1);
    ASSERT_EQ(chunk->size(), chunks_size - m_chunk_header_size + mylib::MemoryArena::PAGE_SIZE);
    // make an arena full
    chunk = arena.get_memory_chunk(1*mylib::MemoryArena::PAGE_SIZE, chunk);
    ASSERT_EQ(arena.remaining(), 0);
    // trying to extend the chunk, but the arena is full, which causes the program to fail
    ASSERT_DEATH(arena.get_memory_chunk(33, chunk), "");
}

TEST_F(ArenaDeathFixture, RunOutOfSpaceWhenCreatingANewChunk) {
    constexpr uint64_t chunk_size = (m_small_arena_size - 2*mylib::MemoryArena::PAGE_SIZE);
    mylib::MemoryArena arena(m_small_arena_size);
    auto* chunk = arena.get_memory_chunk(chunk_size);
    ASSERT_EQ(arena.total_chunks_count(), 1);
    ASSERT_EQ(chunk->size(), (chunk_size - m_chunk_header_size + mylib::MemoryArena::PAGE_SIZE));
    ASSERT_DEATH(arena.get_memory_chunk(mylib::MemoryArena::PAGE_SIZE), "");
}
