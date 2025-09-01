#include "brown_noise.h"
#include "blue_noise.h"
#include <iostream>

BrownNoise::BrownNoise()
{
    threshold_matrix = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    width = 0;
    height = 0;
    leaky_integrator = 0.0;
    output_levels = 0;
    value_min = DBL_MAX;
    value_max = DBL_MIN;
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
    int value = 0;
    int value_min = INT_MAX;
    int value_max = INT_MIN;
    int value_range = 0;
    WhiteNoise white_noise = WhiteNoise(width, height, output_levels);

    white_noise.generate_white_noise();

    std::vector<std::vector<int>> threshold_matrix_white_noise(white_noise.get_threshold_matrix());

    // integrate white noise threshold matrix
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            value = 0;

            for(int j = 0; j < y; j++)
            {
                for(int i = 0; i < x; i++)
                {
                    value += threshold_matrix_white_noise[j][i] * leaky_integrator;
                }
            }

            threshold_matrix[y][x] = value;
            
            if(threshold_matrix[y][x] < value_min)
            {
                value_min = threshold_matrix[y][x];
            }

            if(threshold_matrix[y][x] > value_max)
            {
                value_max = threshold_matrix[y][x];
            }
        }
    }

    value_range = value_max - value_min;

    // normalize to output levels
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            threshold_matrix[y][x] = (threshold_matrix[y][x] - value_min) * (output_levels - 1) / value_range;
        }
    }

    return;
}

// // generates a threshold matrix with brownian noise
// void BrownNoise::generate_brown_noise()
// {
//     std::random_device rd;
//     std::mt19937 mt(rd());
//     std::uniform_real_distribution<> dis(-1.0, 1.0);
//     std::vector<std::vector<double>> threshold_matrix_static_cast<double>(height, std::vector<double>(width, 0.0));
//     double value_previous = 0.0;
//     double value_range = 0.0;

//     // populate temporary double matrix
//     for(int y = 0; y < height; y++)
//     {
//         for(int x = 0; x < width; x++)
//         {
//             threshold_matrix_double[y][x] = leaky_integrator * (value_previous + dis(mt));

//             if(threshold_matrix_double[y][x] < value_min)
//             {
//                 value_min = threshold_matrix_double[y][x];
//             }

//             if(threshold_matrix_double[y][x] > value_max)
//             {
//                 value_max = threshold_matrix_double[y][x];
//             }
//         }
//     }

//     value_range = value_max - value_min;
//     std::cout << value_min << " " << value_max << " " << value_range << std::endl;

//     // normalize double matrix to output levels in int matrix
//     for(int y = 0; y < height; y++)
//     {
//         for(int x = 0; x < width; x++)
//         {
//             threshold_matrix[y][x] = static_cast<int>((threshold_matrix_double[y][x] - value_min) * static_cast<double>(output_levels) / value_range);
//         }
//     }

//     return;
// }

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
