#include "timer.h"
#include <mylib/arena.h>
#include <mylib/growing_array.h>
#include <fmt/core.h>
#include <gtest/gtest.h>

class ArrayFixture : public ::testing::Test {
protected:
    template<std::uint64_t N=1024>
    struct LargeObject {
        static_assert(N >= 1024, "size is too small");
        std::int64_t i64_arr[N];
    };

    template<typename T> 
    void match(const mylib::GrowingArray<T>& a, const std::vector<T>& b) {
        ASSERT_EQ(a.size(), b.size());
        for (auto i = 0; i < a.size(); i++)
            ASSERT_TRUE(a[i] == b[i]);
    }

    constexpr static std::uint64_t ARENA_SIZE = 1024*1024;  
    mylib::Arena m_arena{ARENA_SIZE};
};

using ArrayDeathFixture = ArrayFixture;

TEST_F(ArrayFixture, Create) {
    mylib::GrowingArray<std::string> str_arr(&m_arena);
    ASSERT_EQ(str_arr.size(), 0);
    ASSERT_EQ(str_arr.capacity(), 0);
    constexpr std::uint64_t CAP = 100;
    str_arr.grow(CAP);
    ASSERT_EQ(str_arr.capacity(), 100);
    const std::string s("this is my first string");
    std::vector<std::string> v; v.push_back(s);
    str_arr.push_back(s);
    ASSERT_EQ(str_arr.size(), 1);
    auto& str = str_arr[0];
    ASSERT_EQ(str, s);
    for (std::int32_t index = 0; index < CAP-1; index++) {
        auto s = fmt::format("string with index {}", index);
        str_arr.push_back(s);
        v.push_back(s);
    }
#if 0
    for (auto itr = str_arr.begin(); itr != str_arr.end(); itr++) {
        fmt::print("element: {}", *itr);
    }
#endif
    ASSERT_EQ(str_arr.size(), CAP);
    ASSERT_EQ(str_arr.capacity(), CAP);
    match(str_arr, v);
    ASSERT_EQ(str_arr.front(), s);
    ASSERT_EQ(str_arr.back(), "string with index 98");
    str_arr.pop_back();
    ASSERT_EQ(str_arr.back(), "string with index 97");
    ASSERT_EQ(str_arr.size(), CAP-1);
    str_arr.clear();
    ASSERT_EQ(str_arr.size(), 0);
    ASSERT_EQ(str_arr.max_size(), CAP);
}