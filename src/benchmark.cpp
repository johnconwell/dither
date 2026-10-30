#include "benchmark.h"
#include <algorithm> // std::max
#include <iomanip> // std::setw, std::left

void Benchmark::start()
{
    recorded.clear();
    time_last = std::chrono::steady_clock::now();
}

void Benchmark::step(const std::string& label)
{
    const auto now = std::chrono::steady_clock::now();
    recorded.emplace_back(label, std::chrono::duration_cast<Duration>(now - time_last));
    time_last = now;
}

const std::vector<std::pair<std::string, Benchmark::Duration>>& Benchmark::steps() const
{
    return recorded;
}

Benchmark::Duration Benchmark::total() const
{
    Duration sum{0};

    for(const auto& [label, duration] : recorded)
    {
        sum += duration;
    }

    return sum;
}

void Benchmark::report(std::ostream& out, const std::string& title) const
{
    Duration processing{0};
    bool any_operation = false;
    std::size_t width = std::string("processing").size();

    for(const auto& [label, duration] : recorded)
    {
        width = std::max(width, label.size());

        if(label != "load" && label != "save")
        {
            processing += duration;
            any_operation = true;
        }
    }

    auto line = [&](const std::string& label, const std::string& value) {
        out << "  " << std::left << std::setw(static_cast<int>(width) + 2) << label + ":" << value << "\n";
    };

    out << title << "\n";

    for(const auto& [label, duration] : recorded)
    {
        line(label, format(duration));
    }

    line("processing", any_operation ? format(processing) : "no operations");
    line("total", format(total()));
    out.flush();
}

std::string Benchmark::format(Duration duration)
{
    using namespace std::chrono;
    const auto ns = duration.count();
    const auto us = duration_cast<microseconds>(duration).count();
    const auto ms = duration_cast<milliseconds>(duration).count();

    if(us < 10)
    {
        return std::to_string(ns) + "ns";
    }

    if(ms < 10)
    {
        return std::to_string(us) + "us";
    }

    return std::to_string(ms) + "ms";
}
