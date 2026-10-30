#pragma once

#include <array> // std::array
#include <string> // std::string
#include <string_view> // std::string_view
#include <vector> // std::vector

// Grayscale channel weights: a built-in preset by name, or a csv file with one row "r,g,b".
class Grayscale
{
public:
    using Weights = std::array<double, 3>;

    // throws DitherError for an unknown preset or an invalid file
    explicit Grayscale(const std::string& name);

    static bool has_preset(std::string_view name);
    static std::vector<std::string> preset_names();

    std::string name;
    Weights weights;
};
