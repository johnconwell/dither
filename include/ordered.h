#pragma once

#include <cstddef> // std::size_t
#include <string> // std::string
#include <string_view> // std::string_view
#include <vector> // std::vector

// Ordered dithering threshold matrix (values 0-255): a built-in preset by name, or an
// M x N csv file of whole numbers from 0 to 255.
class Ordered
{
public:
    using Matrix = std::vector<std::vector<double>>;

    // throws DitherError for an unknown preset or an invalid file
    explicit Ordered(const std::string& name);

    static bool has_preset(std::string_view name);
    static std::vector<std::string> preset_names();

    // n x n Bayer matrix (n a power of two), built with the recursive formula and scaled
    // to 0-255: value = index * 256 / n^2
    static Matrix bayer(std::size_t n);

    std::string name;
    Matrix threshold_matrix;
};
