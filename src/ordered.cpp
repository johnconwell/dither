#include "ordered.h"
#include "csv.h"
#include "error.h"
#include "ordered_tables.h"
#include <cmath> // std::floor
#include <sstream> // std::ostringstream

namespace
{
    constexpr std::size_t BAYER_SIZES[] = {2, 4, 8, 16};

    std::string bayer_name(std::size_t n)
    {
        return "BAYER_" + std::to_string(n) + "X" + std::to_string(n);
    }
}

Ordered::Matrix Ordered::bayer(std::size_t n)
{
    // M(2s) = [ 4 M(s) + 0   4 M(s) + 2 ]
    //         [ 4 M(s) + 3   4 M(s) + 1 ]
    constexpr std::size_t QUADRANT[2][2] = {{0, 2}, {3, 1}};
    std::vector<std::vector<std::size_t>> index = {{0}};

    for(std::size_t s = 1; s < n; s *= 2)
    {
        std::vector<std::vector<std::size_t>> next(2 * s, std::vector<std::size_t>(2 * s));

        for(std::size_t y = 0; y < 2 * s; y++)
        {
            for(std::size_t x = 0; x < 2 * s; x++)
            {
                next[y][x] = 4 * index[y % s][x % s] + QUADRANT[y / s][x / s];
            }
        }

        index = std::move(next);
    }

    Matrix matrix(index.size(), std::vector<double>(index.size()));

    for(std::size_t y = 0; y < index.size(); y++)
    {
        for(std::size_t x = 0; x < index.size(); x++)
        {
            matrix[y][x] = static_cast<double>(index[y][x] * 256 / (n * n));
        }
    }

    return matrix;
}

Ordered::Ordered(const std::string& name) : name(name)
{
    for(const std::size_t n : BAYER_SIZES)
    {
        if(name == bayer_name(n))
        {
            threshold_matrix = bayer(n);
            return;
        }
    }

    for(const NoiseMatrix& noise : NOISE_MATRICES)
    {
        if(name == noise.name)
        {
            threshold_matrix.assign(noise.size, std::vector<double>(noise.size));

            for(std::size_t y = 0; y < noise.size; y++)
            {
                for(std::size_t x = 0; x < noise.size; x++)
                {
                    threshold_matrix[y][x] = noise.values[y * noise.size + x];
                }
            }

            return;
        }
    }

    threshold_matrix = csv::read_matrix(name, "threshold matrix");

    for(const auto& row : threshold_matrix)
    {
        for(const double value : row)
        {
            if(value < 0.0 || value > 255.0 || value != std::floor(value))
            {
                std::ostringstream text;
                text << value;
                throw DitherError("threshold matrix value out of range (whole numbers 0-255) in " + name + " - " + text.str());
            }
        }
    }
}

bool Ordered::has_preset(std::string_view name)
{
    for(const std::size_t n : BAYER_SIZES)
    {
        if(name == bayer_name(n))
        {
            return true;
        }
    }

    for(const NoiseMatrix& noise : NOISE_MATRICES)
    {
        if(name == noise.name)
        {
            return true;
        }
    }

    return false;
}

std::vector<std::string> Ordered::preset_names()
{
    std::vector<std::string> names;

    for(const std::size_t n : BAYER_SIZES)
    {
        names.push_back(bayer_name(n));
    }

    for(const NoiseMatrix& noise : NOISE_MATRICES)
    {
        names.emplace_back(noise.name);
    }

    return names;
}
