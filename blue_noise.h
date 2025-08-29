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

class BlueNoise
{
private:
    std::vector<std::vector<int>> dither_array;
    std::vector<std::vector<int>> binary_pattern_initial;
    std::vector<std::vector<int>> binary_pattern_prototype;
    std::vector<std::pair<int, int>> minority_pixels;
    std::vector<std::pair<int, int>> majority_pixels;
    double sigma;
    double coverage;
    std::vector<std::vector<double>> kernel;

    int index(int x, int y, int width, int height);
    double distance_torroidal_squared(std::pair<int, int> p1, std::pair<int, int> p2, int width, int height);
    void load_binary_pattern();
    void invert_binary_pattern();

public:
    BlueNoise();
    BlueNoise(int width, int height, double sigma, double coverage);

    std::vector<std::vector<int>> get_dither_array();

    void kernel_gaussian(double sigma, int width, int height);
    void generate_initial_binary_pattern();
    void generate_dither_array_phase_1();
    void generate_dither_array_phase_2();
    void generate_dither_array_phase_3();
    void normalize_dither_array(int output_levels);
    std::pair<int, int> get_tightest_cluster(std::vector<std::vector<int>> binary_pattern);
    std::pair<int, int> get_largest_void(std::vector<std::vector<int>> binary_pattern);
    double gaussian_value(std::vector<std::vector<int>> binary_pattern, int x, int y);

    std::string to_string_dither_array();
    std::string to_string_binary_pattern_initial();
    std::string to_string_binary_pattern_prototype();
};



#endif
