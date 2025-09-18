#ifndef __BENCHMARK_H
#define __BENCHMARK_H

#include <chrono> // clock_t, clock()

class Benchmark
{
public:
    Benchmark();
    void start();
    void stop();
    std::size_t time_ns();
    std::size_t time_us();
    std::size_t time_ms();
    double time_s();

private:
    std::chrono::time_point<std::chrono::steady_clock> time_start;
    std::chrono::time_point<std::chrono::steady_clock> time_end;
};



#endif
