#ifndef __BROWN_NOISE_H
#define __BROWN_NOISE_H

#include <random>
#include <string>
#include <vector>

class BrownNoise
{
public:
    BrownNoise();
    BrownNoise(int width, int height, double leaky_integrator, int output_levels);
    std::vector<std::vector<int>> get_threshold_matrix();
    void generate_brown_noise();

    std::string to_string();

private:
    std::vector<std::vector<int>> threshold_matrix;
    int width;
    int height;
    double leaky_integrator;
    int output_levels;
};


#endif
