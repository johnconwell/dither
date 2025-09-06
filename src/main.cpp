#include "bayer.h"
#include "blue_noise.h"
#include "brown_noise.h"
#include "dither.h"
#include "error_diffusion.h"
#include "fourier.h"
#include "grayscale.h"
#include "palette.h"
#include "white_noise.h"
#include <chrono>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

std::string get_time_ms_string(clock_t start, clock_t end);
std::string error_diffusion(std::string file_name, Palette palette, ErrorDiffusionAlgorithm algorithm, bool alternate, bool benchmark);
std::string error_diffusion_all(std::string file_name, Palette palette, bool benchmark);
std::string generate_bayer(int size, int output_levels, bool fourier, bool benchmark);
std::string generate_bayer_all(int output_levels, bool fourier, bool benchmark);
std::string generate_blue_noise(int width, int height, double sigma, double coverage, int output_levels, bool fourier, bool benchmark);
std::string generate_blue_noise_all(double sigma, double coverage, int output_levels, bool fourier, bool benchmark);
std::string generate_brown_noise(int width, int height, double leaky_integrator, int output_levels, bool fourier, bool benchmark);
std::string generate_brown_noise_all(double leaky_integrator, int output_levels, bool fourier, bool benchmark);
std::string generate_white_noise(int width, int height, int output_levels, bool fourier, bool benchmark);
std::string generate_white_noise_all(int output_levels, bool fourier, bool benchmark);

int main()
{
    Palette palette_black_white = Palette("BLACK_WHITE", Palette::preset_palettes.at(PresetPalette::BLACK_WHITE));
    Palette palette_1bit_monitor_glow = Palette("_1BIT_MONITOR_GLOW", Palette::preset_palettes.at(PresetPalette::_1BIT_MONITOR_GLOW));
    Palette palette_twilight5 = Palette("_1BIT_MONITOR_GLOW", Palette::preset_palettes.at(PresetPalette::_1BIT_MONITOR_GLOW));

    int output_levels = Color::CHANNEL_MAX + 1;
    double sigma = 1.9;
    double coverage = 0.1;
    double leaky_integrator = 0.999;
    
    std::cout << error_diffusion_all("statue", palette_black_white, true) << std::endl;
    std::cout << generate_bayer_all(output_levels, true, true) << std::endl;
    std::cout << generate_blue_noise_all(sigma, coverage, output_levels, true, true) << std::endl;
    std::cout << generate_brown_noise_all(leaky_integrator, output_levels, true, true) << std::endl;
    std::cout << generate_white_noise_all(output_levels, true, true) << std::endl;

    std::cout << "finished" << std::endl;
    return 0;
}

std::string get_time_ms_string(clock_t start, clock_t end)
{
    return std::to_string(1000.0 * (end - start) / CLOCKS_PER_SEC) + " ms";
}

std::string error_diffusion(std::string file_name, Palette palette, ErrorDiffusionAlgorithm algorithm, bool alternate, bool benchmark)
{
    std::string output = "";
    clock_t time_start;
    clock_t time_end;
    std::string file_path_input = "input\\" + file_name + ".png";
    std::string file_path_output = "output\\error_diffusion\\" + file_name;
    std::string file_path_suffix = "";
    Dither dither = Dither();

    dither.set_palette(palette);
    dither.load(file_path_input.c_str());

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%s %s time: ", ErrorDiffusion::ALGORITHM_STRING.at(algorithm).c_str(), alternate ? "alternate" : "standard");
        output += heading;
        time_start = clock();
    }

    dither.error_diffusion(algorithm, alternate);

    if(benchmark)
    {
        time_end = clock();
        output += get_time_ms_string(time_start, time_end) + "\n";
    }

    dither.save((file_path_output + "_" + ErrorDiffusion::ALGORITHM_STRING.at(algorithm) + ".png").c_str());

    return output;
}

std::string error_diffusion_all(std::string file_name, Palette palette, bool benchmark)
{
    std::string output = "Error Diffusion:\n";

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::LINEAR, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::LINEAR, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::FLOYD_STEINBERG, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::FLOYD_STEINBERG, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::JARVICE_JUDICE_NINKE, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::JARVICE_JUDICE_NINKE, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::STUCKI, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::STUCKI, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::ATKINSON, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::ATKINSON, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::BURKES, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::BURKES, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA_TWO_ROW, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA_TWO_ROW, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA_LITE, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA_LITE, true, benchmark);

    return output;
}

std::string generate_bayer(int size, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "";
    clock_t time_start;
    clock_t time_end;
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\threshold_matrix\\bayer_%ix%i.png", size, size);

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%ix%i time: ", size, size);
        output += heading;
        time_start = clock();
    }

    Bayer bayer = Bayer(size, output_levels);
    bayer.generate_bayer_matrix();

    if(benchmark)
    {
        time_end = clock();
        output += get_time_ms_string(time_start, time_end) + "\n";
    }

    image.create_from_threshold_matrix(bayer.get_threshold_matrix());
    image.save(file_name);

    if(fourier)
    {
        if(benchmark)
        {
            char heading[100];
            sprintf(heading, "%ix%i fourier time: ", size, size);
            output += heading;
            time_start = clock();
        }

        Fourier2D fourier_2d = Fourier2D(bayer.get_threshold_matrix(), true, true);
        fourier_2d.dft();
        fourier_2d.normalize_transform(output_levels);

        if(benchmark)
        {
            time_end = clock();
            output += get_time_ms_string(time_start, time_end) + "\n";
        }

        sprintf(file_name, "output\\threshold_matrix\\bayer_%ix%i_fourier.png", size, size);
        image.create_from_threshold_matrix(fourier_2d.get_transform());
        image.save(file_name);
    }

    return output;
}

std::string generate_bayer_all(int output_levels, bool fourier, bool benchmark)
{
    std::string output = "Bayer:\n";

    output += generate_bayer(2, output_levels, fourier, benchmark);
    output += generate_bayer(4, output_levels, fourier, benchmark);
    output += generate_bayer(8, output_levels, fourier, benchmark);
    output += generate_bayer(16, output_levels, fourier, benchmark);
    output += generate_bayer(32, output_levels, fourier, benchmark);
    output += generate_bayer(64, output_levels, fourier, benchmark);

    return output;
}

std::string generate_blue_noise(int width, int height, double sigma, double coverage, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "";
    clock_t time_start;
    clock_t time_end;
    BlueNoise blue_noise = BlueNoise(width, height, sigma, coverage, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\threshold_matrix\\blue_noise_%ix%i_s%.1f.png", width, height, sigma);

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%ix%i time: ", width, height);
        output += heading;
        time_start = clock();
    }
    
    blue_noise.generate_blue_noise();

    if(benchmark)
    {
        time_end = clock();
        output += get_time_ms_string(time_start, time_end) + "\n";
    }

    image.create_from_threshold_matrix(blue_noise.get_threshold_matrix());
    image.save(file_name);

    if(fourier)
    {
        if(benchmark)
        {
            char heading[100];
            sprintf(heading, "%ix%i fourier time: ", width, height);
            output += heading;
            time_start = clock();
        }

        Fourier2D fourier_2d = Fourier2D(blue_noise.get_threshold_matrix(), true, true);
        fourier_2d.dft();
        fourier_2d.normalize_transform(output_levels);

        if(benchmark)
        {
            time_end = clock();
            output += get_time_ms_string(time_start, time_end) + "\n";
        }

        sprintf(file_name, "output\\threshold_matrix\\blue_noise_%ix%i_s%.1f_fourier.png", width, height, sigma);
        image.create_from_threshold_matrix(fourier_2d.get_transform());
        image.save(file_name);
    }

    return output;
}

std::string generate_blue_noise_all(double sigma, double coverage, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "Blue Noise:\n";

    output += generate_blue_noise(2, 2, sigma, std::max(coverage, 0.25), output_levels, fourier, benchmark);
    output += generate_blue_noise(4, 4, sigma, std::max(coverage, 0.0625), output_levels, fourier, benchmark);
    output += generate_blue_noise(8, 8, sigma, coverage, output_levels, fourier, benchmark);
    output += generate_blue_noise(16, 16, sigma, coverage, output_levels, fourier, benchmark);
    output += generate_blue_noise(32, 32, sigma, coverage, output_levels, fourier, benchmark);
    output += generate_blue_noise(64, 64, sigma, coverage, output_levels, fourier, benchmark);

    return output;
}

std::string generate_brown_noise(int width, int height, double leaky_integrator, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "";
    clock_t time_start;
    clock_t time_end;
    BrownNoise brown_noise = BrownNoise(width, height, leaky_integrator, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\threshold_matrix\\brown_noise_%ix%i.png", width, height);

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%ix%i time: ", width, height);
        output += heading;
        time_start = clock();
    }

    brown_noise.generate_brown_noise();

    if(benchmark)
    {
        time_end = clock();
        output += get_time_ms_string(time_start, time_end) + "\n";
    }

    image.create_from_threshold_matrix(brown_noise.get_threshold_matrix());
    image.save(file_name);

    if(fourier)
    {
        if(benchmark)
        {
            char heading[100];
            sprintf(heading, "%ix%i fourier time: ", width, height);
            output += heading;
            time_start = clock();
        }

        Fourier2D fourier_2d = Fourier2D(brown_noise.get_threshold_matrix(), true, true);
        fourier_2d.dft();
        fourier_2d.normalize_transform(output_levels);

        if(benchmark)
        {
            time_end = clock();
            output += get_time_ms_string(time_start, time_end) + "\n";
        }

        sprintf(file_name, "output\\threshold_matrix\\brown_noise_%ix%i_fourier.png", width, height);
        image.create_from_threshold_matrix(fourier_2d.get_transform());
        image.save(file_name);
    }

    return output;
}

std::string generate_brown_noise_all(double leaky_integrator, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "Brown Noise:\n";

    output += generate_brown_noise(2, 2, leaky_integrator, output_levels, fourier, benchmark);
    output += generate_brown_noise(4, 4, leaky_integrator, output_levels, fourier, benchmark);
    output += generate_brown_noise(8, 8, leaky_integrator, output_levels, fourier, benchmark);
    output += generate_brown_noise(16, 16, leaky_integrator, output_levels, fourier, benchmark);
    output += generate_brown_noise(32, 32, leaky_integrator, output_levels, fourier, benchmark);
    output += generate_brown_noise(64, 64, leaky_integrator, output_levels, fourier, benchmark);

    return output;
}

std::string generate_white_noise(int width, int height, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "";
    clock_t time_start;
    clock_t time_end;
    WhiteNoise white_noise = WhiteNoise(width, height, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\threshold_matrix\\white_noise_%ix%i.png", width, height);

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%ix%i time: ", width, height);
        output += heading;
        time_start = clock();
    }

    white_noise.generate_white_noise();

    if(benchmark)
    {
        time_end = clock();
        output += get_time_ms_string(time_start, time_end) + "\n";
    }

    image.create_from_threshold_matrix(white_noise.get_threshold_matrix());
    image.save(file_name);

    if(fourier)
    {
        if(benchmark)
        {
            char heading[100];
            sprintf(heading, "%ix%i fourier time: ", width, height);
            output += heading;
            time_start = clock();
        }

        Fourier2D fourier_2d = Fourier2D(white_noise.get_threshold_matrix(), true, true);
        fourier_2d.dft();
        fourier_2d.normalize_transform(output_levels);

        if(benchmark)
        {
            time_end = clock();
            output += get_time_ms_string(time_start, time_end) + "\n";
        }

        sprintf(file_name, "output\\threshold_matrix\\white_noise_%ix%i_fourier.png", width, height);
        image.create_from_threshold_matrix(fourier_2d.get_transform());
        image.save(file_name);
    }

    return output;
}

std::string generate_white_noise_all(int output_levels, bool fourier, bool benchmark)
{
    std::string output = "White Noise:\n";

    output += generate_white_noise(2, 2, output_levels, fourier, benchmark);
    output += generate_white_noise(4, 4, output_levels, fourier, benchmark);
    output += generate_white_noise(8, 8, output_levels, fourier, benchmark);
    output += generate_white_noise(16, 16, output_levels, fourier, benchmark);
    output += generate_white_noise(32, 32, output_levels, fourier, benchmark);
    output += generate_white_noise(64, 64, output_levels, fourier, benchmark);

    return output;
}
