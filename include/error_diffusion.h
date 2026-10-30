#pragma once

#include <string> // std::string
#include <string_view> // std::string_view
#include <vector> // std::vector

// Error diffusion kernel: where the quantisation error goes, relative to the current pixel
// (dx to the right in scan direction, dy down), and what share of it goes there.
class ErrorDiffusion
{
public:
    struct Tap
    {
        int dx;
        int dy;
        double scalar;
    };

    // throws DitherError for an unknown algorithm
    explicit ErrorDiffusion(const std::string& name);

    static bool has_preset(std::string_view name);
    static std::vector<std::string> preset_names();

    std::string name;
    std::vector<Tap> taps;
};
