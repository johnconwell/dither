// MIT License
//
// Copyright (c) 2025 John Conwell
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __NOISE2D_H
#define __NOISE2D_H

#include <cmath> // std::max
#include <cstddef> // std::size_t
#include <random> // std::random_device, std::mt19337, std::uniform_int_distribution
#include <vector> // std::vector

template<typename T>
class Noise2D
{
public:
    Noise2D();
    Noise2D(std::size_t width, std::size_t height, std::size_t output_levels = 2);

    T get_noise_at(std::size_t x, std::size_t y);

    void generate_blue_noise(double sigma = 1.9);
    void generate_brown_noise(double leaky_integrator = 0.999);
    void generate_white_noise();

private:
    class EnergyLUT;
    static inline const std::size_t OUTPUT_LEVELS_MIN = 2;
    static inline const double COVERAGE = 0.1;

    std::vector<std::vector<T>> data;
    std::size_t width;
    std::size_t height;
    std::size_t output_levels;

    // blue noise specific members
    void generate_initial_binary_pattern(double sigma);
    void generate_blue_noise_rank_data_phase_1(double sigma);
    void generate_blue_noise_rank_data_phase_2(double sigma);
    void generate_blue_noise_rank_data_phase_3(double sigma);
    void normalize_blue_noise_rank_data();

    void binary_pattern_copy(std::vector<std::vector<int>> &binary_pattern_source, std::vector<std::vector<int>> &binary_pattern_destination);
    void binary_pattern_invert(std::vector<std::vector<int>> &binary_pattern);

    std::vector<std::vector<int>> blue_noise_rank_data;
    std::vector<std::vector<int>> binary_pattern_initial;
    std::vector<std::vector<int>> binary_pattern_prototype;
    EnergyLUT energy_lut;
    double coverage;
};

template<typename T>
class Noise2D<T>::EnergyLUT
{
public:
    EnergyLUT();
    EnergyLUT(size_t width, size_t height);
    void create(std::vector<std::vector<int>> binary_pattern, double sigma);
    void update(std::vector<std::vector<int>> binary_pattern, size_t x, size_t y, double sigma);

    std::vector<std::vector<double>> LUT;
    size_t height;
    size_t width;
    double value_lowest_energy;
    double value_highest_energy;
    std::pair<int, int> coordinate_lowest_energy;
    std::pair<int, int> coordinate_highest_energy;
};

template<typename T>
Noise2D<T>::Noise2D()
{
    data = std::vector<std::vector<T>>(0, std::vector<T>(0, static_cast<T>(0)));
    width = 0;
    height = 0;
    output_levels = OUTPUT_LEVELS_MIN;

    // blue noise specific members
    blue_noise_rank_data = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    binary_pattern_initial = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    binary_pattern_prototype = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    energy_lut = EnergyLUT();
    
    return;
}

template<typename T>
Noise2D<T>::Noise2D(std::size_t width, std::size_t height, std::size_t output_levels)
{
    this->data = std::vector<std::vector<T>>(height, std::vector<T>(width, static_cast<T>(0)));
    this->width = width;
    this->height = height;
    this->output_levels = std::max(OUTPUT_LEVELS_MIN, output_levels);

    // blue noise specific members
    this->blue_noise_rank_data = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->binary_pattern_initial = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->binary_pattern_prototype = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->energy_lut = EnergyLUT(width, height);

    return;
}

template<typename T>
T Noise2D<T>::get_noise_at(std::size_t x, std::size_t y)
{
    return data[y][x];
}

template<typename T>
void Noise2D<T>::generate_blue_noise(double sigma)
{
    generate_initial_binary_pattern();
    generate_blue_noise_rank_data_phase_1();
    generate_blue_noise_rank_data_phase_2();
    generate_blue_noise_rank_data_phase_3();
    normalize_blue_noise_rank_data();
    return;
}

template<typename T>
void Noise2D<T>::generate_brown_noise(double leaky_integrator)
{
    std::vector<std::vector<T>> data_white_noise = std::vector<std::vector<T>>(height, std::vector<T>(width, static_cast<T>(0)));
    std::vector<std::vector<double>> data_temporary = std::vector<std::vector<double>>(height, std::vector<double>(width, 0.0));
    std::random_device rd;
    std::mt19937 mt(rd());
    double input_min = DBL_MAX;
    double input_max = DBL_MIN;
    double integrated_min = DBL_MAX;
    double integrated_max = DBL_MIN;

    // generate initial white noise data to be integrated into brown noise
    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            if constexpr(std::is_integral<T>::value)
            {
                std::uniform_int_distribution<> dis(0, output_levels - 1);
                data_white_noise[y][x] = dis(mt);
            }
            else
            {
                std::uniform_real_distribution<> dis(0, 1);
                data_white_noise[y][x] = dis(mt);
            }

            // get min and max values from white noise threshold matrix
            if(data_white_noise[y][x] < input_min)
            {
                input_min = static_cast<double>(data_white_noise[y][x]);
            }
            if(data_white_noise[y][x] > input_max)
            {
                input_max = static_cast<double>(data_white_noise[y][x]);
            }
        }
    }

    double half_input_range = (input_max - input_min) / 2.0;

    // integrate white noise threshold matrix
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            data_temporary[y][x] = 0.0;

            for(size_t j = 0; j <= y; j++)
            {
                for(size_t i = 0; i <= x; i++)
                {
                    data_temporary[y][x] += (static_cast<double>(data_white_noise[j][i]) - half_input_range) * leaky_integrator;
                }
            }
            
            if(data_temporary[y][x] < integrated_min)
            {
                integrated_min = data_temporary[y][x];
            }

            if(data_temporary[y][x] > integrated_max)
            {
                integrated_max = data_temporary[y][x];
            }
        }
    }

    double integrated_range = integrated_max - integrated_min;

    // normalize to output levels
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            data[y][x] = static_cast<T>((data_temporary[y][x] - integrated_min) * static_cast<double>(output_levels - 1) / integrated_range);
        }
    }

    return;
}

template<typename T>
void Noise2D<T>::generate_white_noise()
{
    std::random_device rd;
    std::mt19937 mt(rd());

    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            if constexpr(std::is_integral<T>::value)
            {
                std::uniform_int_distribution<> dis(0, output_levels - 1);
                data[y][x] = dis(mt);
            }
            else
            {
                std::uniform_real_distribution<> dis(0, 1);
                data[y][x] = dis(mt);
            }
        }
    }

    return;
}

template<typename T>
void Noise2D<T>::generate_initial_binary_pattern(double sigma)
{
    size_t num_pixels = height * width;
    size_t num_minority_pixels = std::max(1, static_cast<size_t>(static_cast<double>(num_pixels) * COVERAGE)); // must have at least 1 minority pixel
    std::vector<std::pair<int, int>> remaining_coordinates = std::vector<std::pair<int, int>>(num_pixels, {-1, -1});
    std::random_device rd;
    std::mt19937 mt(rd());

    // fill remaining_coordinates with all coordinates in the binary pattern matrix
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            // ((y * height) + x) maps 2d array coords to a 1d array
            size_t index_remaining_coordinates = (y * height) + x;
            remaining_coordinates[index_remaining_coordinates].first = x;
            remaining_coordinates[index_remaining_coordinates].second = y;
        }
    }

    // shuffle remaining coordinates array
    for(int index_remaining_coordinates = num_pixels - 1; index_remaining_coordinates >= 0; index_remaining_coordinates--)
    {
        std::uniform_int_distribution<> dis(0, index_remaining_coordinates);
        int index_random = dis(mt);
        std::pair<int, int> temp = remaining_coordinates[index_remaining_coordinates];
        remaining_coordinates[index_remaining_coordinates] = remaining_coordinates[index_random];
        remaining_coordinates[index_random] = temp;
    }

    // fill binary pattern with num_minority_pixels ones
    for(size_t index_remaining_coordinates = 0; index_remaining_coordinates < num_pixels; index_remaining_coordinates++)
    {
        std::pair<int, int> coordinate = remaining_coordinates[index_remaining_coordinates];
        if(index_remaining_coordinates < num_minority_pixels)
        {
            binary_pattern_initial[coordinate.second][coordinate.first] = 1;
        }
    }

    energy_lut.create(binary_pattern_initial, sigma);

    // limiting iterations ensures that this will not get stuck forever if there is a bug
    size_t count = 0;
    while(count < 100)
    {
        // find the tightest cluster and remove it
        std::pair<int, int> coordinate_tightest_cluster = energy_lut.coordinate_highest_energy;
        binary_pattern_initial[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = 0;
        energy_lut.update(binary_pattern_initial, coordinate_tightest_cluster.first, coordinate_tightest_cluster.second, sigma);

        // find the largest void and fill it
        std::pair<int, int> coordinate_largest_void = energy_lut.coordinate_lowest_energy;
        binary_pattern_initial[coordinate_largest_void.second][coordinate_largest_void.first] = 1;
        energy_lut.update(binary_pattern_initial, coordinate_largest_void.first, coordinate_largest_void.second, sigma);

        // exit condition when removing the tightest cluster creates the largest void
        if((coordinate_tightest_cluster.first == coordinate_largest_void.first) && (coordinate_tightest_cluster.second == coordinate_largest_void.second))
        {
            break;
        }

        count++;
    }

    return;
}

template<typename T>
void Noise2D<T>::generate_blue_noise_rank_data_phase_1(double sigma)
{
    int ones = std::max(1, static_cast<int>(static_cast<double>(width * height) * COVERAGE)); // must have at least 1 minority pixel (one)
    int rank = ones - 1;

    binary_pattern_copy(binary_pattern_initial, binary_pattern_prototype);
    energy_lut.create(binary_pattern_prototype, sigma);

    while(rank >= 0)
    {
        std::pair<int, int> coordinate_tightest_cluster = energy_lut.coordinate_highest_energy;
        binary_pattern_prototype[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = 0;
        energy_lut.update(binary_pattern_prototype, coordinate_tightest_cluster.first, coordinate_tightest_cluster.second, sigma);
        blue_noise_rank_data[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = rank;
        rank--;
    }

    return;
}

template<typename T>
void Noise2D<T>::generate_blue_noise_rank_data_phase_2(double sigma)
{
    int num_pixels_half = width * height / 2;
    int ones = std::max(1, static_cast<int>(static_cast<double>(width * height) * COVERAGE)); // must have at least 1 minority pixel (one)
    int rank = ones;

    binary_pattern_copy(binary_pattern_initial, binary_pattern_prototype);
    energy_lut.create(binary_pattern_prototype, sigma);

    while(rank < num_pixels_half)
    {
        std::pair<int, int> coordinate_largest_void = energy_lut.coordinate_lowest_energy;
        binary_pattern_prototype[coordinate_largest_void.second][coordinate_largest_void.first] = 1;
        energy_lut.update(binary_pattern_prototype, coordinate_largest_void.first, coordinate_largest_void.second, sigma);
        blue_noise_rank_data[coordinate_largest_void.second][coordinate_largest_void.first] = rank;
        rank++;
    }

    return;
}

template<typename T>
void Noise2D<T>::generate_blue_noise_rank_data_phase_3(double sigma)
{
    int num_pixels = width * height;
    int num_pixels_half = num_pixels / 2;
    int rank = num_pixels_half;

    binary_pattern_invert(binary_pattern_prototype);
    energy_lut.create(binary_pattern_prototype, sigma);

    while(rank < num_pixels)
    {
        std::pair<int, int> coordinate_tightest_cluster = energy_lut.coordinate_highest_energy;
        binary_pattern_prototype[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = 0;
        energy_lut.update(binary_pattern_prototype, coordinate_tightest_cluster.first, coordinate_tightest_cluster.second, sigma);
        blue_noise_rank_data[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = rank;
        rank++;
    }

    return;
}

template<typename T>
void Noise2D<T>::normalize_blue_noise_rank_data()
{
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            // TODO
            if(std::is_integral<T>::value)
            {

            }
            binary_pattern_destination[y][x] = binary_pattern_source[y][x];
        }
    }

    return;
}

template<typename T>
void Noise2D<T>::binary_pattern_copy(std::vector<std::vector<int>> &binary_pattern_source, std::vector<std::vector<int>> &binary_pattern_destination)
{
    

    return;
}

template<typename T>
void Noise2D<T>::binary_pattern_invert(std::vector<std::vector<int>> &binary_pattern)
{


    return;
}

template<typename T>
Noise2D<T>::EnergyLUT::EnergyLUT()
{
    height = 0;
    width = 0;
    LUT = std::vector<std::vector<double>>(0, std::vector<double>(0, 0.0));
    value_lowest_energy = DBL_MAX;
    value_highest_energy = 0.0;
    coordinate_lowest_energy = std::pair<int, int>(-1, -1);
    coordinate_highest_energy = std::pair<int, int>(-1, -1);
    return;
}

template<typename T>
Noise2D<T>::EnergyLUT::EnergyLUT(size_t width, size_t height)
{
    this->height = height;
    this->width = width;
    LUT = std::vector<std::vector<double>>(height, std::vector<double>(width, 0.0));
    value_lowest_energy = DBL_MAX;
    value_highest_energy = 0.0;
    coordinate_lowest_energy = std::pair<int, int>(-1, -1);
    coordinate_highest_energy = std::pair<int, int>(-1, -1);
    return;
}

template<typename T>
void Noise2D<T>::EnergyLUT::create(std::vector<std::vector<int>> binary_pattern, double sigma)
{
    value_lowest_energy = DBL_MAX;
    value_highest_energy = 0.0;
    double half_width = static_cast<double>(width) / 2.0;
    double half_height = static_cast<double>(height) / 2.0;
    double two_sigma_squared = 2.0 * sigma * sigma;

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            // reset LUT value before calculating energy
            LUT[y][x] = 0.0;

            // calculate the contribution of each pixel in the binary pattern
            for(size_t j = 0; j < height; j++)
            {
                double dy = std::abs(static_cast<double>(y) - static_cast<double>(j));

                if(dy > half_height)
                {
                    dy = height - dy;
                }

                for(size_t i = 0; i < width; i++)
                {
                    // only pixels of value 1 contribute to the energy
                    if(binary_pattern[j][i] == 1)
                    {
                        double dx = std::abs(static_cast<double>(x) - static_cast<double>(i));
                
                        if(dx > half_width)
                        {
                            dx = width - dx;
                        }

                        LUT[y][x] += exp(-1 * ((dx * dx) + (dy * dy)) / two_sigma_squared);
                    }
                }
            }

            // only 0s (majority pixels) in the binary pattern can be considered the largest void
            if(binary_pattern[y][x] == 0 && LUT[y][x] < value_lowest_energy)
            {
                value_lowest_energy = LUT[y][x];
                coordinate_lowest_energy = std::pair<int, int>(x, y);
            }

            // only 1s (minority pixels) in the binary pattern can be considered the largest cluster
            if(binary_pattern[y][x] == 1 && LUT[y][x] > value_highest_energy)
            {
                value_highest_energy = LUT[y][x];
                coordinate_highest_energy = std::pair<int, int>(x, y);
            }
        }
    }

    return;
}

template<typename T>
void Noise2D<T>::EnergyLUT::update(std::vector<std::vector<int>> binary_pattern, size_t x, size_t y, double sigma)
{
    double half_width = static_cast<double>(width) / 2.0;
    double half_height = static_cast<double>(height) / 2.0;
    double two_sigma_squared = 2 * sigma * sigma;

    value_lowest_energy = DBL_MAX;
    value_highest_energy = 0.0;

    // calculate the contribution of each pixel in the binary pattern
    for(size_t j = 0; j < height; j++)
    {
        double dy = std::abs(static_cast<double>(y) - static_cast<double>(j));

        if(dy > half_height)
        {
            dy = height - dy;
        }

        for(size_t i = 0; i < width; i++)
        {
            double dx = std::abs(static_cast<double>(x) - static_cast<double>(i));
    
            if(dx > half_width)
            {
                dx = width - dx;
            }

            double gaussian_value = exp(-1 * ((dx * dx) + (dy * dy)) / two_sigma_squared);

            // if the newly updated pixel is a 1, then add to the other pixels
            if(binary_pattern[y][x] == 1)
            {
                LUT[j][i] += gaussian_value;
            }
            // if the newly updated pixel is a 0, then subtract from the other pixels
            else
            {
                LUT[j][i] -= gaussian_value;
            }

            // only 0s (majority pixels) in the binary pattern can be considered the largest void
            if(binary_pattern[j][i] == 0 && LUT[j][i] < value_lowest_energy)
            {
                value_lowest_energy = LUT[j][i];
                coordinate_lowest_energy = std::pair<int, int>(i, j);
            }

            // only 1s (minority pixels) in the binary pattern can be considered the largest cluster
            if(binary_pattern[j][i] == 1 && LUT[j][i] > value_highest_energy)
            {
                value_highest_energy = LUT[j][i];
                coordinate_highest_energy = std::pair<int, int>(i, j);
            }
        }
    }

    return;
}

#endif
