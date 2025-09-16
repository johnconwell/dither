#ifndef __CONVOLVE_H
#define __CONVOLVE_H

#include <cmath> // acos
#include <vector> // std::vector

template <typename T>
std::vector<std::vector<T>> convolve(std::vector<std::vector<T>> matrix, std::vector<std::vector<T>> kernel)
{
    const std::size_t matrix_height = matrix.size();
    const std::size_t matrix_width = matrix[0].size();
    const std::size_t kernel_height = kernel.size();
    const std::size_t kernel_width = kernel[0].size();
    const std::size_t kernel_height_half = kernel_height / 2;
    const std::size_t kernel_width_half = kernel_width / 2;
    const double kernel_area = static_cast<double>(kernel_height) * static_cast<double>(kernel_width);
    std::vector<std::vector<double>> convoluted_matrix = std::vector<std::vector<double>>(matrix_height, std::vector<double>(matrix_width, 0.0));
    double sum = 0.0;

    for(std::size_t my = 0; my < matrix_height; my++)
    {
        for(std::size_t mx = 0; mx < matrix_width; mx++)
        {
            sum = 0;
            
            for(std::size_t ky = 0; ky < kernel_height; ky++)
            {
                for(std::size_t kx = 0; kx < kernel_width; kx++)
                {
                    std::size_t dy = (matrix_height + ((my + ky - kernel_height_half) % matrix_height)) % matrix_height;
                    std::size_t dx = (matrix_width + ((mx + kx - kernel_width_half) % matrix_width)) % matrix_width;
                    sum += static_cast<double>(matrix[dy][dx]) * static_cast<double>(kernel[ky][kx]);
                }
            }
            
            convoluted_matrix[my][mx] = static_cast<T>(sum / kernel_area);
        }
    }

    return convoluted_matrix;
}

std::vector<std::vector<double>> gaussian_kernel(std::size_t size, double sigma)
{
    std::vector<std::vector<double>> kernel = std::vector<std::vector<double>>(size, std::vector<double>(size, 0.0));

    const double NOISE2D_PI = acos(-1.0);
    const int half_size = size / 2;
    const float two_sigma_squared = 2.0 * sigma * sigma;
    double sum = 0.0;

    for(int y = -half_size; y < half_size + 1; y++)
    {
        for(int x = -half_size; x < half_size + 1; x++)
        {
            kernel[y + half_size][x + half_size] = exp(-1 * (x * x + y * y) / two_sigma_squared) / (NOISE2D_PI * two_sigma_squared);
            sum += kernel[y + half_size][x + half_size];
        }
    }

    for(std::size_t y = 0; y < size; y++)
    {
        for(std::size_t x = 0; x < size; x++)
        {
            kernel[y][x] /= sum;
        }
    }

    return kernel;
}

#endif
