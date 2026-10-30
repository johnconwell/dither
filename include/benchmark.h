#pragma once

#include <chrono> // std::chrono::steady_clock, std::chrono::nanoseconds
#include <ostream> // std::ostream
#include <string> // std::string
#include <utility> // std::pair
#include <vector> // std::vector

// Times a run as a series of named steps ("load", one per operation, "save").
class Benchmark
{
public:
    using Duration = std::chrono::nanoseconds;

    // starts (or restarts) the clock and forgets any recorded steps
    void start();
    // records the time since the previous step (or since start) under `label`
    void step(const std::string& label);

    const std::vector<std::pair<std::string, Duration>>& steps() const;
    Duration total() const;

    // every step, then "processing" (every step except load and save) and "total"
    void report(std::ostream& out, const std::string& title) const;

    // 812ns, 45us or 81ms: the largest unit that still gives at least 10
    static std::string format(Duration duration);

private:
    std::chrono::steady_clock::time_point time_last;
    std::vector<std::pair<std::string, Duration>> recorded;
};
