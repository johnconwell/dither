#pragma once

#include <string> // std::string
#include <string_view> // std::string_view
#include <vector> // std::vector

// Convolution kernel: a built-in preset by name, or an M x N csv file.
class Convolve
{
public:
    using Kernel = std::vector<std::vector<double>>;

    // throws DitherError for an unknown preset or an invalid file
    explicit Convolve(const std::string& name);

    static bool has_preset(std::string_view name);
    static std::vector<std::string> preset_names();

    std::string name;
    Kernel kernel;
};
