#pragma once
#include <chrono>

namespace test
{
class Timer {
public:
    Timer();
    ~Timer();
private:
    std::chrono::high_resolution_clock::time_point m_start;
};
} // namespace test
