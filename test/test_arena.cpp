#include <containers/growing_array.h>
#include <containers/arena.h>
#include <gtest/gtest.h>

namespace test 
{

template<class T>
class GrowingArray {
public:
    class const_iterator;
    class iterator;

    GrowingArray(ml::MemoryArena *arena)
    : m_arena(arena) {}

    void push_back(const T& obj) {
        m_arena->push(obj);
    }

    void push_back(T&& obj) {
        m_arena->push(std::move(obj));
    }

    void pop_back() const {
        m_arena->pop();
    }

private:
    ml::MemoryArena *m_arena;
};

} // namespace test

TEST(Arena, Creation) {
    ml::MemoryArena arena;
    test::GrowingArray<int32_t> arr(&arena);

    arr.push_back(2344);
    arr.push_back(344);
    arr.push_back(-2344);
}