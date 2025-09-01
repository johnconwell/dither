#include "white_noise.h"

WhiteNoise::WhiteNoise()
{
    threshold_matrix = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    width = 0;
    height = 0;
    output_levels = 0;
    return;
}

WhiteNoise::WhiteNoise(size_t width, size_t height, size_t output_levels)
{
    this->threshold_matrix = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->width = width;
    this->height = height;
    this->output_levels = output_levels;
    return;
}

std::vector<std::vector<int>> WhiteNoise::get_threshold_matrix()
{
    return threshold_matrix;
}

void WhiteNoise::generate_white_noise()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> dis(0, output_levels - 1);

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            threshold_matrix[y][x] = dis(mt);
        }
    }

    return;
}

std::string WhiteNoise::to_string()
{
    std::string output = "";

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(threshold_matrix[y][x]) + " ";
        }
        
        output += "\n";
    }

    return output;
}
