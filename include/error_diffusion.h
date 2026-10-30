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

    // throws DitherError for an unknown algorithm; an alias such as JARVICE_JUDICE_NINKE is accepted
    explicit ErrorDiffusion(const std::string& name);

    // true for preset names and their aliases
    static bool has_preset(std::string_view name);
    // preset names only, without aliases
    static std::vector<std::string> preset_names();

    // the preset name, with any alias resolved
    std::string name;
    std::vector<Tap> taps;
};
