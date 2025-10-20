#ifndef __CLI_H
#define __CLI_H

#include "cxxopts.hpp"
#include <cstdlib> // std::exit
#include <iostream> // std::cerr, std::endl
#include <string> // std::string

class CLI
{
public:
    CLI();
    bool parse(int argc, const char* argv[]);
    std::string to_string();

    std::string file_path_input;
    std::string file_path_output;
    bool gamma_correction;
    bool grayscale;
    std::string grayscale_weights;
    bool convolve;
    std::string convolve_kernel;
    bool reduce;
    bool error_diffusion;
    std::string error_diffusion_algorithm;
    std::string error_diffusion_scan_pattern;
    bool ordered;
    std::string ordered_threshold_matrix;
    bool temporal;
    std::string temporal_method;
    std::string temporal_frames;
    std::string mapping_method;
    std::string palette;
    bool benchmark;
};

#endif
