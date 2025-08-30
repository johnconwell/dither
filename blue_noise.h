#ifndef __BLUE_NOISE_H
#define __BLUE_NOISE_H

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <float.h>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct EnergyLUT
{
    EnergyLUT();
    EnergyLUT(int width, int height);
    void create(std::vector<std::vector<int>> binary_pattern, double sigma);
    void update(std::vector<std::vector<int>> binary_pattern, int x, int y, double sigma);

    std::string to_string();

    std::vector<std::vector<double>> LUT;
    int height;
    int width;
    double value_lowest_energy;
    double value_highest_energy;
    std::pair<int, int> coordinate_lowest_energy;
    std::pair<int, int> coordinate_highest_energy;
};

class BlueNoise
{
public:
    BlueNoise();
    BlueNoise(int width, int height, double sigma, double coverage, int output_levels);

    std::vector<std::vector<int>> get_dither_array();
    void generate_dither_array();

    std::string to_string_dither_array();
    std::string to_string_binary_pattern_initial();
    std::string to_string_binary_pattern_prototype();

private:
    std::vector<std::vector<int>> dither_array;
    std::vector<std::vector<int>> binary_pattern_initial;
    std::vector<std::vector<int>> binary_pattern_prototype;
    EnergyLUT energy_lut;
    int height;
    int width;
    double sigma;
    double coverage;
    int output_levels;

    void generate_initial_binary_pattern();
    void generate_dither_array_phase_1();
    void generate_dither_array_phase_2();
    void generate_dither_array_phase_3();
    void normalize_dither_array();

    void binary_pattern_copy(std::vector<std::vector<int>> &binary_pattern_source, std::vector<std::vector<int>> &binary_pattern_destination);
    void binary_pattern_invert(std::vector<std::vector<int>> &binary_pattern);
};



#endif
