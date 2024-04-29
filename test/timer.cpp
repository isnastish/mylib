#include "timer.h"
#include <iostream>

Timer::Timer()
    : m_start(std::chrono::high_resolution_clock::now())
{}

Timer::~Timer()
{
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start);
    std::cout << "Took: " << elapsed.count() << "(ns)" << std::endl;
}
