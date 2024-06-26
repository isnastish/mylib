#include "arena.h"

#include <fmt/core.h>
#include <stdexcept>

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# undef max
# undef min
#endif

namespace
{
class allocation_error : public std::exception {
public:
    explicit allocation_error(const char* msg) noexcept 
    : std::exception(msg)
    {}
};
}

namespace mylib
{

Arena::Arena(std::uint64_t size) {
    m_blocks.push_back(std::make_unique<MemBlock>(size));
    m_blocks_count += 1;
}

Arena::Arena(Arena&& rhs) 
: m_blocks(std::move(rhs.m_blocks)),
  m_blocks_count(rhs.m_blocks_count) {}

Arena& Arena::operator=(Arena&& rhs) {
    if (this == &rhs) return *this;
    m_blocks = std::move(rhs.m_blocks);
    m_blocks_count = rhs.m_blocks_count;
    return *this;
}

Chunk* Arena::getChunk(std::uint64_t size, Chunk* old_chunk) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // NOTE: Compute a total allocation size taking into consideration an alignment.
    // If a chunk is nullptr, we include the size of the MemBlock::ChunkHeader into the total allocation size.
    // Thus, if requested size is equal to 1024, which is exactly the size of a single page (PAGE_SIZE), 
    // two pages will be allocated comprising the size of 2048bytes(2Kib) in total,
    // because we have to fit a ChunkHeader struct.
    // +--------+-------------------+------------------------------------------------------------+
    // | chunk  |         size      |                      empty space                           |
    // | header |         size      |                      empty space                           |
    // +--------+-------------------+------------------------------------------------------------+
    //                              ^
    //                              |
    //                             m_pos
    std::uint64_t new_size = size + CHUNK_HEADER_SIZE;
    std::uint64_t rem = new_size % Arena::PAGE_SIZE;
    std::uint64_t new_aligned_size = (rem == 0) ? new_size : (new_size + Arena::PAGE_SIZE - rem);
    std::uint64_t page_count = new_aligned_size / Arena::PAGE_SIZE;
    std::uint64_t total_size = page_count * Arena::PAGE_SIZE;

    MemBlock* potential_block = nullptr;
    for (auto itr = m_blocks.begin(); itr != m_blocks.end(); itr++) {
        MemBlock* cur_block = itr->get();
        auto result = cur_block->getEmptyChunk(total_size);
        if (result.has_value()) {
            return result.value();
        }
        
        if (!potential_block && (cur_block->remainingSpace() > total_size)) {
            potential_block = cur_block;
        }
    }

    // Arena where to insert a chunk hasn't been found.
    if (!potential_block) {
        std::uint64_t new_size = std::max(total_size, DEFAULT_ALLOC_SIZE);
        auto itr = m_blocks.insert(m_blocks.end(), std::make_unique<MemBlock>(new_size));
        potential_block = itr->get();
        m_blocks_count += 1;
    }

    auto* new_chunk = potential_block->newChunk(total_size);

    // NOTE: Presumably, this shouldn't be the responsibility of arena to copy the data.
    // The one who owns the old chunk should copy before releasing it.
    // new_chunk = arena->getChunk(old_chunk->size() * 2);
    // new_chunk->copy(old_chunk);
    // arena->releaseChunk(old_chunk);
    // But maybe it's convenient to keep it here.
    if (old_chunk) {
        new_chunk->copy(old_chunk);
        potential_block->freeChunk(old_chunk);
    }

    return new_chunk;
}

void Arena::releaseChunk(Chunk* chunk) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_blocks.empty() && chunk) {
        for (auto itr = m_blocks.begin(); itr != m_blocks.end(); itr++) {
            Arena::MemBlock* block = itr->get();
            if (block->freeChunk(chunk)) 
                return;
        }
    }
}

std::uint64_t Arena::emptyChunksCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::uint64_t empty_chunks = 0;
    for (auto itr = m_blocks.begin(); itr != m_blocks.end(); itr++) {
        empty_chunks += itr->get()->emptyChunksCount();
    }
    return empty_chunks;
}

std::uint64_t Arena::totalChunks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::uint64_t total_chunks = 0;
    for (auto itr = m_blocks.begin(); itr != m_blocks.end(); itr++) {
        total_chunks += itr->get()->totalChunks();
    }
    return total_chunks;
}

std::uint64_t Arena::totalBlocks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_blocks.size();
}

Arena::MemBlock::MemBlock(std::uint64_t size) {
    std::uint64_t arena_size = sizeof(Arena::MemBlock); 
    std::uint64_t new_size = size + arena_size;
    std::uint64_t rem = new_size % PAGE_SIZE; 
    new_size += (rem == 0 ? 0 : PAGE_SIZE - rem);
    m_ptr = static_cast<std::byte*>(MemBlock::allocMemory(new_size));
    m_cap = new_size;
    m_pos = 0;
}

Arena::MemBlock::~MemBlock() {
    if (m_ptr) {
        releaseMemory(m_ptr);
    }
}

Chunk* Arena::MemBlock::newChunk(std::uint64_t size) noexcept {
    auto* chunk_pair = reinterpret_cast<Arena::MemBlock::ChunkHeader*>(m_ptr + m_pos);
    std::byte* start = m_ptr + CHUNK_HEADER_SIZE;
    std::uint64_t chunk_size = size - CHUNK_HEADER_SIZE;
    chunk_pair->chunk = Chunk(start, chunk_size);
    chunk_pair->state = Arena::MemBlock::ChunkState::IN_USE;
    if (m_chunks) {
        chunk_pair->next = m_chunks;
        chunk_pair->prev = m_chunks->prev;
        m_chunks->prev->next = chunk_pair;
        m_chunks->prev = chunk_pair;
    }
    else {
        m_chunks = chunk_pair;
        m_chunks->next = m_chunks;
        m_chunks->prev = m_chunks;
    }

    m_pos += size;
    m_total_chunks_count += 1;

    return &chunk_pair->chunk;
}

bool Arena::MemBlock::freeChunk(Chunk* chunk) noexcept {
    if (chunk && m_chunks) {
        if (m_chunks == m_chunks->prev && (&m_chunks->chunk == chunk)) {
            m_chunks->state = ChunkState::FREE;
            m_chunks->chunk.reset();
            m_empty_chunks_count += 1;
            return true;
        }

        for (auto* chunk_header = m_chunks;;
            chunk_header = chunk_header->next) {
            if ((chunk == &chunk_header->chunk) && (chunk_header->state == ChunkState::IN_USE)) {
                chunk_header->state = ChunkState::FREE;
                chunk_header->chunk.reset();
                m_empty_chunks_count += 1;
                return true;
            }

            if (chunk_header == m_chunks->prev) {
                break;
            }
        }
    }
    
    return false;
}

std::optional<Chunk*> Arena::MemBlock::getEmptyChunk(std::uint64_t size) noexcept {
    if (m_chunks) {
        const std::uint64_t chunk_size = size - CHUNK_HEADER_SIZE;
        ChunkHeader* chunk_header = nullptr;

        for (auto* cur_header = m_chunks;; 
            cur_header = cur_header->next) {
            if ((cur_header->state == ChunkState::FREE) && 
                (cur_header->chunk.size() >= chunk_size)) {
                if (!chunk_header)
                    chunk_header = cur_header;
                else {
                    if (cur_header->chunk.size() < chunk_header->chunk.size())
                        chunk_header = cur_header;
                }
            }

            if (cur_header == m_chunks->prev) {
                break;
            }
        }

        if (chunk_header) {
            chunk_header->state = ChunkState::IN_USE;
            m_empty_chunks_count -= 1;
            return {&chunk_header->chunk};
        }
    }

    return {}; 
}

std::uint64_t Arena::MemBlock::totalChunks() const noexcept {
    return m_total_chunks_count;
}

std::uint64_t Arena::MemBlock::emptyChunksCount() const noexcept {
    return m_empty_chunks_count;
}

std::uint64_t Arena::MemBlock::remainingSpace() const noexcept {
    return (m_cap - m_pos);
}

void* Arena::MemBlock::allocMemory(uint64_t size) {
#ifdef _WIN32
    void *memory = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#else
    void *memory = malloc(size);
    std::memset(memory, 0, size);
#endif
    if (!memory) 
        throw allocation_error(fmt::format("failed to allocate memory block of size {}", size).c_str());
    return memory;
}

void Arena::MemBlock::releaseMemory(void *memory) {
#ifdef _WIN32
    VirtualFree(memory, 0, MEM_RELEASE);
#else
    free(memory);
#endif
}

Chunk::Chunk(std::byte* start, std::uint64_t size) noexcept
: m_start(start), m_size(size), m_pos(0)
{}

void Chunk::copy(const Chunk* src_chunk) {
    if (m_size < src_chunk->size())
        throw std::length_error(fmt::format("destination chunk doesn't have enough space, required {}", src_chunk->size()));
    std::memcpy(m_start, src_chunk->m_start, src_chunk->size());
    m_pos = src_chunk->m_pos;
}

void Chunk::pop(std::uint64_t size) { 
    if (m_pos < size)
        throw std::length_error(fmt::format("the size to be deducted {} exceeds the current size {}", size, m_pos));
    m_pos -= size;
}

void Chunk::reset() noexcept { 
    std::memset(m_start, 0, m_pos); 
    m_pos = 0;
}

void Chunk::doesFit(std::uint64_t size) const {
    if ((m_pos + size) > m_size)
        throw std::length_error(fmt::format("not enough space to insert an object of size {}", size));
}

} // namespace mylib