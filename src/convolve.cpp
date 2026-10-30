#include "convolve.h"
#include "csv.h"
#include <unordered_map> // std::unordered_map

namespace
{
    // built once, on first use, and shared
    const std::unordered_map<std::string, Convolve::Kernel>& presets()
    {
        static const std::unordered_map<std::string, Convolve::Kernel> PRESETS = {
            {"RIDGE_4", {
                {+0.0, -1.0, +0.0},
                {-1.0, +4.0, -1.0},
                {+0.0, -1.0, +0.0},
            }},
            {"RIDGE_8", {
                {-1.0, -1.0, -1.0},
                {-1.0, +8.0, -1.0},
                {-1.0, -1.0, -1.0},
            }},
            {"SHARPEN_4", {
                {+0.0, -1.0, +0.0},
                {-1.0, +5.0, -1.0},
                {+0.0, -1.0, +0.0},
            }},
            {"SHARPEN_8", {
                {-1.0, -1.0, -1.0},
                {-1.0, +9.0, -1.0},
                {-1.0, -1.0, -1.0},
            }},
            {"BOX_BLUR", {
                {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
                {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
                {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
            }},
            {"GAUSSIAN_BLUR", {
                {0.00296902, 0.0133062, 0.0219382, 0.0133062, 0.00296902},
                {0.0133062, 0.0596343, 0.0983203, 0.0596343, 0.0133062},
                {0.0219382, 0.0983203, 0.162103, 0.0983203, 0.0219382},
                {0.0133062, 0.0596343, 0.0983203, 0.0596343, 0.0133062},
                {0.00296902, 0.0133062, 0.0219382, 0.0133062, 0.00296902},
            }},
            {"UNSHARP_MASK", {
                {-0.00296902, -0.0133062, -0.0219382, -0.0133062, -0.00296902},
                {-0.0133062, -0.0596343, -0.0983203, -0.0596343, -0.0133062},
                {-0.0219382, -0.0983203, +1.837897, -0.0983203, -0.0219382},
                {-0.0133062, -0.0596343, -0.0983203, -0.0596343, -0.0133062},
                {-0.00296902, -0.0133062, -0.0219382, -0.0133062, -0.00296902},
            }},
        };

        return PRESETS;
    }
}

Convolve::Convolve(const std::string& name) : name(name)
{
    if(const auto it = presets().find(name); it != presets().end())
    {
        kernel = it->second;
    }
    else
    {
        kernel = csv::read_matrix(name, "convolution kernel");
    }
}

bool Convolve::has_preset(std::string_view name)
{
    return presets().contains(std::string(name));
}

std::vector<std::string> Convolve::preset_names()
{
    std::vector<std::string> names;

    for(const auto& [name, _] : presets())
    {
        names.push_back(name);
    }

    return names;
}
