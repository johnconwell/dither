#ifndef __BLUE_NOISE_H
#define __BLUE_NOISE_H

#include <cfloat> // DBL_MIN, DBL_MAX
#include <cmath> // std::abs()
#include <random> // std::random_device, std::mt19937, std::uniform_int_distribution
#include <string> // std::string, std::to_string()
#include <utility> // std::pair
#include <vector> // std::vector

struct EnergyLUT
{
    EnergyLUT();
    EnergyLUT(size_t width, size_t height);
    void create(std::vector<std::vector<int>> binary_pattern, double sigma);
    void update(std::vector<std::vector<int>> binary_pattern, size_t x, size_t y, double sigma);

    std::string to_string();

    std::vector<std::vector<double>> LUT;
    size_t height;
    size_t width;
    double value_lowest_energy;
    double value_highest_energy;
    std::pair<int, int> coordinate_lowest_energy;
    std::pair<int, int> coordinate_highest_energy;
};

class BlueNoise
{
public:
    BlueNoise();
    BlueNoise(size_t width, size_t height, double sigma, double coverage, size_t output_levels);
    std::vector<std::vector<int>> get_threshold_matrix();
    void generate_blue_noise();

    std::string to_string_dither_array();
    std::string to_string_binary_pattern_initial();
    std::string to_string_binary_pattern_prototype();

private:
    std::vector<std::vector<int>> dither_array;
    std::vector<std::vector<int>> binary_pattern_initial;
    std::vector<std::vector<int>> binary_pattern_prototype;
    EnergyLUT energy_lut;
    size_t height;
    size_t width;
    double sigma;
    double coverage;
    size_t output_levels;

    void generate_initial_binary_pattern();
    void generate_dither_array_phase_1();
    void generate_dither_array_phase_2();
    void generate_dither_array_phase_3();
    void normalize_dither_array();

    void binary_pattern_copy(std::vector<std::vector<int>> &binary_pattern_source, std::vector<std::vector<int>> &binary_pattern_destination);
    void binary_pattern_invert(std::vector<std::vector<int>> &binary_pattern);
};

#endif
