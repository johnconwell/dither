#include "cli.h"
#include "dither.h"
#include "image.h"
#include "palette.h"
#include <cstdlib> // std::exit
#include <iostream>

int main(int argc, const char* argv[])
{
    CLI cli = CLI();

    if(!cli.parse(argc, argv))
    {
        return EXIT_FAILURE;
    }

    Dither dither = Dither();
    std::size_t error = dither.load(cli.file_path_input.c_str());

    if(error)
    {
        return EXIT_FAILURE;
    }

    if(cli.grayscale)
    {
        dither.grayscale(cli.grayscale_weights, cli.gamma_correction);
    }

    if(cli.convolve)
    {
        dither.convolve(cli.convolve_kernel);
    }

    if(cli.reduce)
    {
        dither.reduce(cli.mapping_method, cli.palette, cli.gamma_correction);
    }

    if(cli.error_diffusion)
    {
        dither.error_diffusion(cli.error_diffusion_algorithm, cli.error_diffusion_scan_pattern, cli.mapping_method, cli.palette, cli.gamma_correction);
    }

    if(cli.ordered)
    {
        dither.ordered(cli.ordered_threshold_matrix, cli.mapping_method, cli.palette, cli.gamma_correction);
    }

    if(cli.temporal)
    {
        dither.temporal(cli.temporal_method, std::stoi(cli.temporal_frames), cli.mapping_method, cli.palette, cli.gamma_correction);
    }

    error = dither.save(cli.file_path_output.c_str());

    if(error)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
