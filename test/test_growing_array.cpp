#include <containers/growing_array.h>
#include "timer.h"
#include <gtest/gtest.h>
#include <vector>
#include <iostream>

#define Print(c) _display(#c, c)

namespace test
{
constexpr size_t SIZE = 4096;

// TODO: Consider alignment as well.

/*
* Memory blob for testing
*/
class Blob {
public:
    Blob(size_t alloc_size=256, size_t allignment=4)
    : cap(alloc_size), ptr(new uint8_t[]{}){}
    ~Blob() { delete []ptr; }

    Blob(const Blob& rhs)
    : cap(rhs.cap), ptr(new uint8_t[]{})
    { memcpy(ptr, rhs.ptr, rhs.cap); }
    Blob& operator=(const Blob& rhs) { 
        if (this == &rhs) return *this;
        uint8_t* new_data = new uint8_t[rhs.cap]{};
        memcpy(new_data, rhs.ptr, rhs.cap);
        uint8_t* tmp = ptr;
        cap = rhs.cap;
        ptr = new_data;
        delete []tmp;
        return *this;
    }

    Blob(Blob&& rhs)
    : cap(rhs.cap), ptr(rhs.ptr) 
    { zero(); }

    Blob& operator=(Blob&& rhs) {
        if (this == &rhs) return *this;
        uint8_t* tmp = ptr;
        cap = rhs.cap;
        ptr = rhs.ptr;
        rhs.zero();
        delete []tmp;
        return *this;
    }

    bool grow(size_t new_cap) {
        if (new_cap > cap) {
            uint8_t *p = new uint8_t[new_cap]{};
            auto copy_size = std::distance(at, ptr); 
            memcpy(p, ptr, copy_size);
            uint8_t *tmp = ptr;
            ptr = p;
            delete []tmp; // free the last.
            return true;
        }
        return false;
    }

private:
    void zero() { cap = 0; ptr = nullptr; }

    size_t cap;
    uint8_t *ptr;
    uint8_t *at;
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
    ml::GrowingArray<test::Blob> arr;
    arr.grow(test::SIZE);
    for (size_t i = 0; i < test::SIZE; i++) {
        arr.push_back(test::Blob());
    }
    ASSERT_EQ(arr.size(), test::SIZE);
}

#undef Print