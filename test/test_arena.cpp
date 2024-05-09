#include <mylib/arena.h>
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <thread>
#include <functional> // std::mem_fn

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
    std::vector<mylib::Chunk*> chunks;
    constexpr std::uint64_t CHUNKS_COUNT = 15;
    for (int i = 1; i <= CHUNKS_COUNT; i++) {
        std::uint64_t chunk_size = (i*mylib::Arena::PAGE_SIZE - mylib::Arena::CHUNK_HEADER_SIZE); 
        chunks.push_back(arena.getChunk(chunk_size));
    }
    ASSERT_EQ(arena.totalBlocks(), 2);
    ASSERT_EQ(arena.totalChunks(), CHUNKS_COUNT);
    // At this point we have 15 chunks with the state ::IN_USE which are spread 
    // out across two separate memory blocks, one of size 10Kib, and another one
    // of size 1Gib. What we want to test if the ability of arena to search for the 
    // most optimal chunk across multiple memory blocks.
    std::for_each(chunks.begin(), chunks.end(), [&arena](mylib::Chunk* chunk){
        if (chunk->size() <= 13*mylib::Arena::PAGE_SIZE) {
            arena.releaseChunk(chunk);
        }
    });
    ASSERT_EQ(arena.emptyChunksCount(), 13);
    auto* chunk = arena.getChunk(12*mylib::Arena::PAGE_SIZE - mylib::Arena::CHUNK_HEADER_SIZE); // 12Kib
    ASSERT_EQ(arena.emptyChunksCount(), 12);
    ASSERT_EQ(chunk->size(), 12*mylib::Arena::PAGE_SIZE - mylib::Arena::CHUNK_HEADER_SIZE);
    arena.releaseChunk(chunk);
    ASSERT_EQ(arena.emptyChunksCount(), 13);
}

struct ChunkStorage {
    std::vector<mylib::Chunk*> chunks;
    std::mutex mu;
};

TEST_F(ArenaFixture, ThreadSafety) {
    constexpr std::uint64_t CHUNK_SIZE = (mylib::Arena::PAGE_SIZE - mylib::Arena::CHUNK_HEADER_SIZE);
    constexpr std::uint64_t CHUNK_COUNT = 5;

    ChunkStorage storage;
    auto createChunks = [&storage](mylib::Arena* arena) {
        // We shouldn't protect access to the arena itself while getting the chunk, 
        // because then we won't be able to test whether it's thread-safe or not. 
        // On the other hand, storage of chunks has to be protected.
        for (std::int32_t i = 0; i < CHUNK_COUNT; i++) {
            auto* chunk = arena->getChunk(CHUNK_SIZE);
            storage.mu.lock();
            storage.chunks.push_back(chunk);
            storage.mu.unlock();
        }
    };

    // Iterate through all the chunks and release if they satisfy the size requirement. 
    // Essentially, we have to test whether releasing chunks is thread-safe, 
    // meaning that the same chunk cannot be released twice by multiple threads.
    // We shouldn't lock storage's mutex, since we only iterating over the vector.
    auto releaseChunks = [&storage](mylib::Arena* arena) {
        for (auto itr = storage.chunks.begin(); 
            itr != storage.chunks.end();
            itr++) {
            arena->releaseChunk(*itr);
        }
    };

    mylib::Arena arena(m_small_arena_size);
    constexpr std::int32_t THREAD_COUNT = 10;
    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);
    for (std::int32_t i = 1; i <= THREAD_COUNT; i++) {
        threads.push_back(std::thread(createChunks, &arena));
    }
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    ASSERT_EQ(arena.totalBlocks(), 2);
    ASSERT_EQ(arena.totalChunks(), THREAD_COUNT*CHUNK_COUNT);
    ASSERT_EQ(arena.emptyChunksCount(), 0);

    // release memory chunks
    threads.clear();
    for (std::int32_t i = 1; i <= THREAD_COUNT; i++) {
        threads.push_back(std::thread(releaseChunks, &arena));
    }
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    ASSERT_EQ(arena.emptyChunksCount(), THREAD_COUNT*CHUNK_COUNT);
}