#include "benchmark.h"

Benchmark::Benchmark()
{
	time_start = std::chrono::time_point<std::chrono::steady_clock>();
    time_end = std::chrono::time_point<std::chrono::steady_clock>();
}

void Benchmark::start()
{
	time_start = std::chrono::steady_clock::now();
}

void Benchmark::stop()
{
	time_end = std::chrono::steady_clock::now();
}

std::size_t Benchmark::time_ns()
{
	return std::chrono::nanoseconds(time_end - time_start).count();
}

std::size_t Benchmark::time_us()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count();
}

std::size_t Benchmark::time_ms()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

double Benchmark::time_s()
{
	return std::chrono::duration<double>(time_end - time_start).count();
}
