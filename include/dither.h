#ifndef __DITHER_H
#define __DITHER_H

#include "convolve.h"
#include "error_diffusion.h"
#include "grayscale.h"
#include "image.h"
#include "ordered.h"
#include "palette.h"
#include <algorithm> // std::clamp
#include <random> // std::random_device, std::mt19337, std::uniform_reaL_distribution
#include <unordered_map> // std::unordered_map
#include <vector> // std::vector

class Dither
{
public:
    Dither(std::size_t frames = 1);

    std::size_t load(const char* file_name);
    std::size_t save(const char* file_name);

    void grayscale(std::string name_grayscale_weights, bool gamma_correction);
    void convolve(std::string name_kernel);
    void reduce(std::string name_mapping_method, std::string name_palette, bool gamma_correction);
    void error_diffusion(std::string name_algorithm, std::string name_mapping_method, std::string name_palette, bool gamma_correction);
    void ordered(std::string name_threshold_matrix, std::string name_mapping_method, std::string name_palette, bool gamma_correction);
    void temporal(std::string name_temporal_method, std::size_t frames, std::string name_mapping_method, std::string name_palette, bool gamma_correction);

    Image image;
};

template <typename T, typename K>
std::vector<std::vector<T>> _convolve(std::vector<std::vector<T>> matrix, std::vector<std::vector<K>> kernel)
{
    const std::size_t matrix_height = matrix.size();
    const std::size_t matrix_width = matrix[0].size();
    const std::size_t kernel_height = kernel.size();
    const std::size_t kernel_width = kernel[0].size();
    const std::size_t kernel_height_half = kernel_height / 2;
    const std::size_t kernel_width_half = kernel_width / 2;
    std::vector<std::vector<T>> convolved_matrix = std::vector<std::vector<T>>(matrix_height, std::vector<T>(matrix_width, 0.0));
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
            
            convolved_matrix[my][mx] = static_cast<T>(sum);
        }
    }

    return convolved_matrix;
}

#endif
