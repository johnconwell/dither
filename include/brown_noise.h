#ifndef __BROWN_NOISE_H
#define __BROWN_NOISE_H

#include "white_noise.h" // for generating white noise seed, which is integrated to get brown noise
#include <cfloat> // DBL_MIN, DBL_MAX
#include <climits> // INT_MIN, INT_MAX
#include <random> // std::random_device, std::mt19937, std::uniform_int_distribution
#include <string> // std::string, std::to_string()
#include <vector> // std::vector

class BrownNoise
{
public:
    BrownNoise();
    BrownNoise(size_t width, size_t height, double leaky_integrator, size_t output_levels);
    std::vector<std::vector<int>> get_threshold_matrix();
    void generate_brown_noise();

    std::string to_string();

private:
    std::vector<std::vector<int>> threshold_matrix;
    size_t width;
    size_t height;
    double leaky_integrator;
    size_t output_levels;
    int input_min;
    int input_max;
    double integrated_min;
    double integrated_max;
    int normalized_min;
    int normalized_max;
};


#endif
