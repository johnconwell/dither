#include "brown_noise.h"

BrownNoise::BrownNoise()
{
    threshold_matrix = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    width = 0;
    height = 0;
    leaky_integrator = 0.0;
    output_levels = 0;
    input_min = INT_MAX;
    input_max = INT_MIN;
    integrated_min = DBL_MAX;
    integrated_max = DBL_MIN;
    return;
}

BrownNoise::BrownNoise(size_t width, size_t height, double leaky_integrator, size_t output_levels)
{
    this->threshold_matrix = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->width = width;
    this->height = height;
    this->leaky_integrator = leaky_integrator;
    this->output_levels = output_levels;
    this->input_min = INT_MAX;
    this->input_max = INT_MIN;
    this->integrated_min = DBL_MAX;
    this->integrated_max = DBL_MIN;
    return;
}

std::vector<std::vector<int>> BrownNoise::get_threshold_matrix()
{
    return threshold_matrix;
}

// generates a threshold matrix with brownian noise
void BrownNoise::generate_brown_noise()
{
    WhiteNoise white_noise = WhiteNoise(width, height, output_levels);

    white_noise.generate_white_noise();

    std::vector<std::vector<int>> threshold_matrix_white_noise(white_noise.get_threshold_matrix());

    // get min and max values from white noise threshold matrix
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            if(threshold_matrix_white_noise[y][x] < input_min)
            {
                input_min = threshold_matrix_white_noise[y][x];
            }

            if(threshold_matrix_white_noise[y][x] > input_max)
            {
                input_max = threshold_matrix_white_noise[y][x];
            }
        }
    }

    int integrated_value = 0;
    int half_input_range = (input_max - input_min) / 2;

    // integrate white noise threshold matrix
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            integrated_value = 0;

            for(size_t j = 0; j < y; j++)
            {
                for(size_t i = 0; i < x; i++)
                {
                    integrated_value += (threshold_matrix_white_noise[j][i] - half_input_range) * leaky_integrator;
                }
            }

            threshold_matrix[y][x] = integrated_value;
            
            if(threshold_matrix[y][x] < integrated_min)
            {
                integrated_min = threshold_matrix[y][x];
            }

            if(threshold_matrix[y][x] > integrated_max)
            {
                integrated_max = threshold_matrix[y][x];
            }
        }
    }

    int integrated_range = integrated_max - integrated_min;

    // normalize to output levels
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            threshold_matrix[y][x] = (threshold_matrix[y][x] - integrated_min) * (output_levels - 1) / integrated_range;
        }
    }

    return;
}

std::string BrownNoise::to_string()
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
