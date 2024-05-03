// TODO: Introduce arena fixture and replace all the tests with a fixture.

#include <containers/growing_array.h>
#include <containers/arena.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

class ArenaFixture : public ::testing::Test {
protected:
    constexpr static uint64_t small_arena_size{64};
    constexpr static uint64_t medium_arena_size{1024*1024};
    constexpr static uint64_t large_arena_size{1024*1024*1024};
};

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

TEST(Arena, GetFreeChunk) {
    constexpr uint64_t size = 1024*1024;
    constexpr uint64_t chunk_1_size = ml::MemoryArena::PAGE_SIZE*5;
    ml::MemoryArena arena(size);
    auto *chunk_1 = arena.get_memory_chunk(nullptr, chunk_1_size);
    /**
     * 5 * PAGE_SIZE = 5*1024 = 5120
     * 5120 + 24(Chunk Header) = 5144
     * aligned by PAGE_SIZE = 6144 (6 pages) total allocated size.
    */
    ASSERT_EQ(arena.remaining(), (size - (chunk_1_size + ml::MemoryArena::PAGE_SIZE)));
    auto *chunk_2 = arena.get_memory_chunk(nullptr, 1000);
    ASSERT_EQ(arena.remaining(), (size - (chunk_1_size + 2*ml::MemoryArena::PAGE_SIZE)));
    arena.release_memory_chunk(chunk_1);
    ASSERT_EQ(arena.empty_chunks_count(), 1);
    chunk_1 = arena.get_memory_chunk(nullptr, chunk_1_size);
    ASSERT_EQ(arena.total_chunks_count(), 3);
    ASSERT_EQ(arena.empty_chunks_count(), 1);
    
    arena.release_memory_chunk(chunk_1);
    arena.release_memory_chunk(chunk_2);
}
