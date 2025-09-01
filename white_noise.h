#ifndef __WHITE_NOISE_H
#define __WHITE_NOISE_H

#include <random>
#include <string>
#include <vector>


class WhiteNoise
{
public:
    WhiteNoise();
    WhiteNoise(int width, int height, int output_levels);
    std::vector<std::vector<int>> get_threshold_matrix();
    void generate_white_noise();

    std::string to_string();

private:
    std::vector<std::vector<int>> threshold_matrix;
    int width;
    int height;
    int output_levels;
};


#endif
