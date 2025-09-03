#ifndef __BROWN_NOISE_H
#define __BROWN_NOISE_H

#include "white_noise.h"
#include <cfloat>
#include <random>
#include <string>
#include <vector>

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
