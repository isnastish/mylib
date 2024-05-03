#include <containers/growing_array.h>
#include <containers/arena.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class ArenaFixture : public ::testing::Test {
protected:
    constexpr static uint64_t m_small_arena_size{64};
    constexpr static uint64_t m_medium_arena_size{1024*1024};
    constexpr static uint64_t m_large_arena_size{1024*1024*1024};
    constexpr static uint64_t m_chunk_header_size{sizeof(ml::MemoryChunk)};
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

TEST_F(ArenaFixture, GetSingleChunk) {
    ml::MemoryArena arena(m_medium_arena_size);
    ml::MemoryChunk *chunk = arena.get_memory_chunk(nullptr, 32);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - ml::MemoryArena::PAGE_SIZE);
    ASSERT_EQ(arena.total_chunks_count(), 1);
    arena.release_memory_chunk(chunk);
    ASSERT_EQ(arena.empty_chunks_count(), 1);
}

TEST_F(ArenaFixture, GetMultipleChunks) {
    ml::MemoryArena arena(m_medium_arena_size);
    ml::MemoryChunk *chunk_1 = arena.get_memory_chunk(nullptr, 64);
    ASSERT_EQ(arena.total_chunks_count(), 1);
    ml::MemoryChunk *chunk_2 = arena.get_memory_chunk(nullptr, 1025);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - ml::MemoryArena::PAGE_SIZE*3);
    ASSERT_EQ(arena.total_chunks_count(), 2);

    arena.release_memory_chunk(chunk_1);
    arena.release_memory_chunk(chunk_2);
    ASSERT_EQ(arena.empty_chunks_count(), 2);
}

TEST_F(ArenaFixture, GetFreeChunk) {
    constexpr uint64_t chunk_1_size = ml::MemoryArena::PAGE_SIZE*5;
    ml::MemoryArena arena(m_medium_arena_size);
    auto *chunk_1 = arena.get_memory_chunk(nullptr, chunk_1_size);
    /**
     * 5 * PAGE_SIZE = 5*1024 = 5120
     * 5120 + 24(Chunk Header) = 5144
     * aligned by PAGE_SIZE = 6144 (6 pages) total allocated size.
    */
    ASSERT_EQ(arena.remaining(), (m_medium_arena_size - (chunk_1_size + ml::MemoryArena::PAGE_SIZE)));
    auto *chunk_2 = arena.get_memory_chunk(nullptr, 1000);
    ASSERT_EQ(arena.remaining(), (m_medium_arena_size - (chunk_1_size + 2*ml::MemoryArena::PAGE_SIZE)));
    arena.release_memory_chunk(chunk_1);
    ASSERT_EQ(arena.empty_chunks_count(), 1);
    chunk_1 = arena.get_memory_chunk(nullptr, chunk_1_size);
    ASSERT_EQ(arena.total_chunks_count(), 3);
    ASSERT_EQ(arena.empty_chunks_count(), 1);
    arena.release_memory_chunk(chunk_1);
    arena.release_memory_chunk(chunk_2);
}

TEST_F(ArenaFixture, ExtendChunk) {
    constexpr uint64_t chunk_size = 1024;
    ml::MemoryArena arena(m_medium_arena_size);
    auto *chunk = arena.get_memory_chunk(nullptr, chunk_size);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - (2*ml::MemoryArena::PAGE_SIZE));
    // extend the current chunk
    chunk = arena.get_memory_chunk(chunk, chunk_size);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - (3*ml::MemoryArena::PAGE_SIZE)); 
    ASSERT_EQ(chunk->size(), (chunk_size*3) - m_chunk_header_size);
    chunk = arena.get_memory_chunk(chunk, 64);
    ASSERT_EQ(arena.remaining(), m_medium_arena_size - (4*ml::MemoryArena::PAGE_SIZE));
    ASSERT_EQ(chunk->size(), (chunk_size*4) - m_chunk_header_size);
}

#if 0
TEST_F(ArenaFixture, FindOptimalFreeChunk) {
    constexpr uint64_t chunk_size = 1024;
    ml::MemoryArena arena(m_medium_arena_size);
    std::vector<ml::MemoryChunk*> chunks;
    constexpr int chunks_count = 10;
    chunks.reserve(chunks_count);
    uint64_t total_size = 0;
    for (int i = 1; i <= chunks_count; i++) {
        uint64_t alloc_size = chunk_size*i; 
        total_size += (alloc_size) + (ml::MemoryArena::PAGE_SIZE - (alloc_size % ml::MemoryArena::PAGE_SIZE));
        chunks.push_back(arena.get_memory_chunk(nullptr, alloc_size));
    }
    std::cout << "total size: " << total_size << std::endl;
    // ASSERT_EQ(arena.remaining(), m_medium_arena_size - expected_total_size);
}
#endif