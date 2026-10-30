#pragma once

#include "options.h" // MappingMethod, ScanPattern, TemporalMethod
#include <cstddef> // std::size_t
#include <string> // std::string

// Command line arguments. Enum-like values are parsed and validated here, once, so bad input
// is rejected before any work is done (REVIEW B6, §2.3 point 1).
class CLI
{
public:
    // throws DitherError for invalid arguments; for -h, prints the help and sets help
    void parse(int argc, const char* argv[]);
    std::string to_string() const;

    static inline constexpr std::size_t TEMPORAL_FRAMES_MAX = 256;

    bool help = false;
    std::string file_path_input;
    std::string file_path_output;
    bool gamma_correction = false;
    bool grayscale = false;
    std::string grayscale_weights;
    bool convolve = false;
    std::string convolve_kernel;
    bool reduce = false;
    bool error_diffusion = false;
    std::string error_diffusion_algorithm;
    ScanPattern error_diffusion_scan_pattern = ScanPattern::RASTER;
    bool ordered = false;
    std::string ordered_threshold_matrix;
    bool temporal = false;
    TemporalMethod temporal_method = TemporalMethod::PWM;
    std::size_t temporal_frames = 0;
    // used by whichever one of -r, -e, -d and -t was given; combining them is rejected (D2)
    MappingMethod mapping_method = MappingMethod::EUCLIDEAN_DISTANCE;
    std::string palette;
    bool benchmark = false;
};
