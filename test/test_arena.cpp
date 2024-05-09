#include <mylib/arena.h>
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <thread>

class ArenaFixture : public ::testing::Test {
protected:
    constexpr static std::uint64_t m_small_arena_size{1024u*10};
    constexpr static std::uint64_t m_medium_arena_size{1024u*1024u};
    constexpr static std::uint64_t m_large_arena_size{1024u*1024u*1024u};
};


TEST_F(ArenaFixture, Creation) {
    mylib::Arena arena;
    auto* chunk = arena.getChunk(1024);
    ASSERT_EQ(arena.totalChunks(), 1);
    arena.releaseChunk(chunk);
    ASSERT_EQ(arena.emptyChunksCount(), 1);
}

TEST_F(ArenaFixture, AlignChunkSize) {
    constexpr uint64_t requested_size = 256;
    constexpr uint64_t page_size_minus_header = 
        (mylib::Arena::PAGE_SIZE - mylib::Arena::CHUNK_HEADER_SIZE); 
    mylib::Arena arena(m_medium_arena_size);
    auto* chunk = arena.getChunk(requested_size);
    ASSERT_EQ(chunk->size(), page_size_minus_header);
    auto* chunk2 = arena.getChunk(1024);
    ASSERT_EQ(chunk2->size(), (2*mylib::Arena::PAGE_SIZE) - mylib::Arena::CHUNK_HEADER_SIZE);
    auto* chunk3 = arena.getChunk(page_size_minus_header);
    ASSERT_EQ(chunk3->size(), page_size_minus_header);
    auto* chunk4 = arena.getChunk(0);
    ASSERT_EQ(chunk4->size(), page_size_minus_header);
    ASSERT_EQ(arena.totalChunks(), 4);
}

TEST_F(ArenaFixture, ForceArenaToGrow) {
    mylib::Arena arena(m_small_arena_size);
    std::vector<mylib::Chunk*> chunks;
    constexpr std::int32_t CHUNKS_COUNT = 4;  
    chunks.reserve(CHUNKS_COUNT);
    for (int i = 0; i < CHUNKS_COUNT; i++) {
        chunks.push_back(arena.getChunk(mylib::Arena::PAGE_SIZE));
    }
    ASSERT_EQ(arena.totalBlocks(), 1);
    ASSERT_EQ(arena.totalChunks(), CHUNKS_COUNT);
    auto* chunk = arena.getChunk(2*mylib::Arena::PAGE_SIZE);
    ASSERT_EQ(arena.totalBlocks(), 2);
    ASSERT_EQ(chunk->size(), (3*mylib::Arena::PAGE_SIZE - mylib::Arena::CHUNK_HEADER_SIZE));
    // release chunks
    std::for_each(chunks.begin(), chunks.end(), [&arena](mylib::Chunk* chunk){ 
        arena.releaseChunk(chunk); 
    });
    ASSERT_EQ(arena.emptyChunksCount(), CHUNKS_COUNT);
}

TEST_F(ArenaFixture, RequestChunkGreaterThanArenaSize) {
    // Another arena will be created of an appropriate size, 
    // and a chunk will be allocated from that arena instead.
    // The following tests requests a chunk of a size 1Mib, 
    // which would be aligned by 1Mib and 1Kib, since the chunk header will occupie size as well.
    constexpr std::uint64_t chunk_size = mylib::Arena::PAGE_SIZE*1024; // 1mib
    mylib::Arena arena(m_small_arena_size);
    ASSERT_EQ(arena.totalBlocks(), 1);
    // force arena to create a new memory block
    auto* chunk = arena.getChunk(chunk_size);
    ASSERT_EQ(arena.totalBlocks(), 2);
    ASSERT_EQ(arena.totalChunks(), 1);
}

TEST_F(ArenaFixture, SelectTheMostOptimalChunk) {
    mylib::Arena arena(m_medium_arena_size); // 1Mib
    std::vector<mylib::Chunk*> chunks;
    constexpr std::int32_t CHUNKS_COUNT = 10;
    chunks.reserve(CHUNKS_COUNT);
    for (int i = 1; i <= CHUNKS_COUNT; i++) {
        std::uint64_t chunk_size = (i*mylib::Arena::PAGE_SIZE - mylib::Arena::CHUNK_HEADER_SIZE);
        chunks.push_back(arena.getChunk(chunk_size));
    }
    ASSERT_EQ(arena.totalChunks(), CHUNKS_COUNT);
    ASSERT_EQ(arena.emptyChunksCount(), 0);
    std::for_each(chunks.begin(), chunks.end(), [&arena](mylib::Chunk* chunk){
        if (chunk->size() <= 6*mylib::Arena::PAGE_SIZE) {
            arena.releaseChunk(chunk);
        }
    });
    // Empty chunks have sizes (no excluding chunk header size): 
    // 1024, 2048, 3072, 4096, 5112, 6144, which is 6 chunks in total. 
    ASSERT_EQ(arena.emptyChunksCount(), 6);
    // Get a chunk with the size 3000 bytes (~3Kib) 
    // In this case the most optimal chunks has to be selected, 
    // which would be the chunk with the size 3072bytes
    constexpr std::uint64_t chunk_size = 3000; 
    auto* chunk = arena.getChunk(chunk_size);
    ASSERT_EQ(arena.emptyChunksCount(), 5);
    ASSERT_EQ(chunk->size(), 3*mylib::Arena::PAGE_SIZE - mylib::Arena::CHUNK_HEADER_SIZE);
    // Get one more chunk with exactly the same size,
    // but since the chunk with the size of 3072bytes is already occupied, 
    // the one with the size 4096bytes will be selected, which is the most optimal in this case.
    auto* chunk2 = arena.getChunk(chunk_size);
    ASSERT_EQ(arena.emptyChunksCount(), 4);
    ASSERT_EQ(chunk2->size(), 4*mylib::Arena::PAGE_SIZE - mylib::Arena::CHUNK_HEADER_SIZE);
}

TEST_F(ArenaFixture, SelectMostOptimalChunkAcrossMultipleMemoryBlocks) {
    mylib::Arena arena(m_small_arena_size);
}

TEST_F(ArenaFixture, ThreadSafety) {

}

