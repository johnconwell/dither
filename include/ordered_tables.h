#pragma once

#include <array> // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t
#include <string_view> // std::string_view

// Built-in noise threshold matrices, stored as bytes in src/ordered_tables.cpp.
// Internal to Ordered; use Ordered(name) rather than this table.
struct NoiseMatrix
{
    std::string_view name;
    std::size_t size; // the matrix is size x size
    const std::uint8_t* values; // row by row
};

extern const std::array<NoiseMatrix, 16> NOISE_MATRICES;
