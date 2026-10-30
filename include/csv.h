#pragma once

#include <string> // std::string
#include <string_view> // std::string_view
#include <vector> // std::vector

namespace csv
{
    using Matrix = std::vector<std::vector<double>>;

    // Reads a numeric matrix from a csv file: one row per line, values separated by commas.
    // Shared by convolution kernels, threshold matrices and grayscale weights (ROADMAP 4.4).
    // - blank lines (including trailing ones) and Windows line endings are ignored
    // - spaces around values and a leading '+' are allowed
    // - every row must have the same number of values
    // Throws DitherError naming `what` (for example "convolution kernel") and the file.
    Matrix read_matrix(const std::string& path, std::string_view what);
}
