#include "brown_noise.h"

BrownNoise::BrownNoise()
{
    threshold_matrix = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    width = 0;
    height = 0;
    leaky_integrator = 0.0;
    output_levels = 0;
    return;
}

BrownNoise::BrownNoise(int width, int height, double leaky_integrator, int output_levels)
{
    this->threshold_matrix = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->width = width;
    this->height = height;
    this->leaky_integrator = leaky_integrator;
    this->output_levels = output_levels;
    return;
}

std::vector<std::vector<int>> BrownNoise::get_threshold_matrix()
{
    return threshold_matrix;
}

// generates a threshold matrix with brownian noise
void BrownNoise::generate_brown_noise()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);
    double value_previous = 0.0;

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            threshold_matrix[y][x] = leaky_integrator * (value_previous + dis(mt));
        }
    }

    return;
}

std::string BrownNoise::to_string()
{
    std::string output = "";

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            output += std::to_string(threshold_matrix[y][x]) + " ";
        }
        
        output += "\n";
    }

    return output;
}
