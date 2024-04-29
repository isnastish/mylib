#include <containers/growing_array.h>
#include "timer.h"
#include <gtest/gtest.h>
#include <vector>
#include <iostream>

class GrowingArrayFixture : public ::testing::Test
{
};

#define Print(c) _display(#c, c)

template<class Container>
static void _display(const char *name, const Container& c) {
    std::cout << name << ": {";
    for (auto itr = c.begin(); itr != c.end(); itr++) {
        if (itr == (c.end() - 1)) std::cout << *itr;
        else std::cout << *itr << ", ";
    }
    std::cout << "};" << std::endl;
}

TEST_F(GrowingArrayFixture, Creation) {
    Timer t;
    std::vector<int32_t> vec;
    enum { N = 5 };
    vec.reserve(N);
    for (int32_t i = 0; i < N; i++)
        vec.push_back((int32_t)std::pow(i, 2));

    ml::GrowingArray<int32_t> arr;
    arr.grow(N);
    arr.push_back(1234);
    arr.push_back(444);
    arr.insert(arr.begin(), vec.begin(), vec.end());
    // Print(arr);
    ASSERT_EQ(arr.size(), 7);
}
#undef Print