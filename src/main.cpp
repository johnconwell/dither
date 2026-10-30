#include "benchmark.h"
#include "cli.h"
#include "dither.h"
#include "error.h"
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <exception> // std::exception
#include <iostream> // std::cout, std::cerr

namespace
{
    int run(int argc, const char* argv[])
    {
        CLI cli;
        cli.parse(argc, argv);

        if(cli.help)
        {
            return EXIT_SUCCESS;
        }

        // operations always run in this order, whatever order the flags were given in;
        // CLI allows at most one of reduce, error diffusion, ordered and temporal
        Benchmark benchmark;
        benchmark.start();
        Dither dither;
        dither.load(cli.file_path_input);
        benchmark.step("load");

        if(cli.grayscale)
        {
            dither.grayscale(cli.grayscale_weights, cli.gamma_correction);
            benchmark.step("grayscale");
        }

        if(cli.convolve)
        {
            dither.convolve(cli.convolve_kernel, cli.gamma_correction);
            benchmark.step("convolve");
        }

        if(cli.reduce)
        {
            dither.reduce(cli.mapping_method, cli.palette, cli.gamma_correction);
            benchmark.step("reduce");
        }

        if(cli.error_diffusion)
        {
            dither.error_diffusion(cli.error_diffusion_algorithm, cli.error_diffusion_scan_pattern, cli.mapping_method, cli.palette, cli.gamma_correction);
            benchmark.step("error_diffusion");
        }

        if(cli.ordered)
        {
            dither.ordered(cli.ordered_threshold_matrix, cli.mapping_method, cli.palette, cli.gamma_correction);
            benchmark.step("ordered");
        }

        if(cli.temporal)
        {
            dither.temporal(cli.temporal_method, cli.temporal_frames, cli.mapping_method, cli.palette, cli.gamma_correction);
            benchmark.step("temporal");
        }

        dither.save(cli.file_path_output);
        benchmark.step("save");

        // reported only once the output has been saved successfully
        if(cli.benchmark)
        {
            benchmark.report(std::cout, cli.file_path_output);
        }

        return EXIT_SUCCESS;
    }
}

int main(int argc, const char* argv[])
{
    try
    {
        return run(argc, argv);
    }
    catch(const DitherError& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: unexpected failure - " << e.what() << std::endl;
    }

    return EXIT_FAILURE;
}
