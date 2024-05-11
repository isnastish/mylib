#include "timer.h"
#include <mylib/arena.h>
#include <mylib/growing_array.h>
#include <fmt/core.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <fstream>

class ArrayFixture : public ::testing::Test {
protected:
    mylib::Arena m_arena{1024*1024}; // 1Mib
};

using ArrayDeathFixture = ArrayFixture;

TEST_F(ArrayFixture, CreateEmpty) {
    mylib::GrowingArray<std::string> array(&m_arena);
    array.push_back("This is the first element");
    ASSERT_EQ(array.size(), 1);
}
