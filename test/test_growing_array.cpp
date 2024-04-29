#include <containers/growing_array.h>
#include "timer.h"
#include <gtest/gtest.h>
#include <vector>
#include <iostream>

#define Print(c) _display(#c, c)

namespace test
{
constexpr size_t SIZE = 4096;

/*
* Memory blob for testing
*/
class Blob {
public:
    Blob(size_t size=256)
    : size_(size), data_(new uint8_t[]{}){}
    ~Blob() { delete []data_; }

    Blob(const Blob& rhs)
    : size_(rhs.size_), data_(new uint8_t[]{})
    { memcpy(data_, rhs.data_, rhs.size_); }
    Blob& operator=(const Blob& rhs) { 
        if (this == &rhs) return *this;
        uint8_t* new_data = new uint8_t[rhs.size_]{};
        memcpy(new_data, rhs.data_, rhs.size_);
        uint8_t* tmp = data_;
        size_ = rhs.size_;
        data_ = new_data;
        delete []tmp;
        return *this;
    }

    Blob(Blob&& rhs)
    : size_(rhs.size_), data_(rhs.data_) 
    { zero(); }

    Blob& operator=(Blob&& rhs) {
        if (this == &rhs) return *this;
        uint8_t* tmp = data_;
        size_ = rhs.size_;
        data_ = rhs.data_;
        rhs.zero();
        delete []tmp;
        return *this;
    }
private:
    void zero() { size_ = 0; data_ = nullptr; }

    size_t size_;
    uint8_t *data_;
};

template<class Container>
static void _display(const char *name, const Container& c) {
    std::cout << name << ": {";
    for (auto itr = c.begin(); itr != c.end(); itr++) {
        if (itr == (c.end() - 1)) std::cout << *itr;
        else std::cout << *itr << ", ";
    }
    std::cout << "};" << std::endl;
}

template<class T>
static void matchElements(const ml::GrowingArray<T>& a, const std::vector<T>& b) {
    ASSERT_EQ(a.size(), b.size());
    for (int32_t i = 0; i < b.size(); i++) {
        ASSERT_EQ(a[i], b[i]);
    }
}

} // namespace test

TEST(GrowingArray, Grow) {
    test::Timer t;
    ml::GrowingArray<int32_t> arr;
    arr.grow(test::SIZE);
    ASSERT_EQ(arr.capacity(), test::SIZE);
}

TEST(GrowingArray, PushBack) {
    test::Timer time;
    ml::GrowingArray<int32_t> arr;
    arr.grow(test::SIZE);
    std::vector<int32_t> vec;
    vec.reserve(test::SIZE);
    for (int i = 0; i < test::SIZE; i++) {
        int num = i << 2;
        arr.push_back(num);
        vec.push_back(num);
    }
    test::matchElements(arr, vec);
}

TEST(GrowingArray, PushBackLargeObject) {
    test::Timer time;
}

#undef Print