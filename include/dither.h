#ifndef __DITHER_H
#define __DITHER_H

#include "convolve.h"
#include "error_diffusion.h"
#include "grayscale.h"
#include "image.h"
#include "ordered.h"
#include "palette.h"
#include <algorithm> // std::clamp
#include <cmath> // std::lround
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
    void error_diffusion(std::string name_algorithm, std::string scan_pattern, std::string name_mapping_method, std::string name_palette, bool gamma_correction);
    void ordered(std::string name_threshold_matrix, std::string name_mapping_method, std::string name_palette, bool gamma_correction);
    void temporal(std::string name_temporal_method, std::size_t frames, std::string name_mapping_method, std::string name_palette, bool gamma_correction);

    Image image;
};

#endif
