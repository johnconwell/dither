#pragma once

#include "image.h"
#include "options.h"
#include <cstddef> // std::size_t
#include <string> // std::string

// Facade over the image operations. Preset names may also be csv file paths (see README).
// Every method throws DitherError on bad input.
class Dither
{
public:
    void load(const std::string& file_name);
    void save(const std::string& file_name) const;

    void grayscale(const std::string& name_grayscale_weights, bool gamma_correction);
    void convolve(const std::string& name_kernel);
    void reduce(MappingMethod mapping_method, const std::string& name_palette, bool gamma_correction);
    void error_diffusion(const std::string& name_algorithm, ScanPattern scan_pattern, MappingMethod mapping_method, const std::string& name_palette, bool gamma_correction);
    void ordered(const std::string& name_threshold_matrix, MappingMethod mapping_method, const std::string& name_palette, bool gamma_correction);
    // mapping_method must be EUCLIDEAN_DISTANCE or MANHATTAN_DISTANCE; frames must be at least 1
    void temporal(TemporalMethod temporal_method, std::size_t frames, MappingMethod mapping_method, const std::string& name_palette, bool gamma_correction);

    Image image;
};
