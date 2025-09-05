#include "bayer.h"
#include "blue_noise.h"
#include "brown_noise.h"
#include "dither.h"
#include "error.h"
#include "fourier.h"
#include "grayscale.h"
#include "ordered.h"
#include "palette.h"
#include "white_noise.h"
#include <chrono>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>

void error_diffusion_all(std::string file_name, Palette palette);
void generate_bayer(int size, int output_levels);
void generate_bayer_all(int output_levels);
void generate_blue_noise(int width, int height, double sigma, double coverage, int output_levels);
void generate_blue_noise_all(double sigma, double coverage, int output_levels);
void generate_brown_noise(int width, int height, double leaky_integrator, int output_levels);
void generate_brown_noise_all(double leaky_integrator, int output_levels);
void generate_white_noise(int width, int height, int output_levels);
void generate_white_noise_all(int output_levels);

int main()
{
    Palette palette_black_white = Palette("BLACK_WHITE", Palette::preset_palettes.at(PresetPalette::BLACK_WHITE));
    Palette palette_1bit_monitor_glow = Palette("_1BIT_MONITOR_GLOW", Palette::preset_palettes.at(PresetPalette::_1BIT_MONITOR_GLOW));
    Palette palette_twilight5 = Palette("_1BIT_MONITOR_GLOW", Palette::preset_palettes.at(PresetPalette::_1BIT_MONITOR_GLOW));

    int output_levels = Color::CHANNEL_MAX + 1;
    double sigma = 1.9;
    double coverage = 0.1;
    double leaky_integrator = 0.999;
    
    // error_diffusion_all("statue", palette_black_white);
    // generate_bayer_all(output_levels);
    // generate_blue_noise_all(sigma, coverage, output_levels);
    // generate_brown_noise_all(leaky_integrator, output_levels);
    // generate_white_noise_all(output_levels);

    // fourier();
    // int size = 64;
    // int scalar = 100;
    // std::vector<int> sin_wave = std::vector<int>(size, 0);
    // std::cout << "sin" << std::endl;
    // for(int index = 0; index < size; index++)
    // {
    //     sin_wave[index] = static_cast<int>(scalar * sin(index) + 0.5 * scalar * sin(index * 2) + 2 * scalar * sin(index * 3));
    //     std::cout << index << " " << sin_wave[index] << std::endl;
    // }

    // std::vector<int> transform = fourier_1d(sin_wave, true);

    // std::cout << "transform" << std::endl;
    // for(int index = 0; index < size; index++)
    // {
    //     std::cout << index << " " << transform[index] << std::endl;
    // }

    size_t height = 32;
    size_t width = 32;
    int num_pixels = height * width;
    size_t scalar = 100;
    std::vector<std::vector<int>> array = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    int array_min = INT_MAX;
    int array_max = INT_MIN;
    WhiteNoise wn = WhiteNoise(width, height, output_levels);
    wn.generate_white_noise();
    BlueNoise bn = BlueNoise(width, height, sigma, coverage, output_levels);
    bn.generate_blue_noise();
    array = bn.get_threshold_matrix();

    std::cout << "array" << std::endl;

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            // array[y][x] = static_cast<int>(scalar * sin(3 * y * 2 * std::numbers::pi / height)) + 0.5 * scalar * cos(5 * x * 2 * std::numbers::pi / width);
            array_min = std::min(array_min, array[y][x]);
            array_max = std::max(array_max, array[y][x]);
            std::cout << array[y][x] << " ";
        }

        std::cout << std::endl;
    }

    

    std::vector<std::vector<int>> transform = fourier_transform_2d(array, true, false);

    // transform[0][0] = 0.0;
    // transform[0][0] = 0.0;
    
    int transform_min = INT_MAX;
    int transform_max = INT_MIN;

    std::cout << "transform" << std::endl;
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            transform_min = std::min(transform_min, transform[y][x]);
            transform_max = std::max(transform_max, transform[y][x]);
            std::cout << transform[y][x] << " ";
        }

        std::cout << std::endl;
    }

    int array_range = array_max - array_min;
    int transform_range = transform_max - transform_min;

    // normalize to output levels
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            array[y][x] = (array[y][x] - array_min) * (output_levels - 1) / array_range;
            transform[y][x] = (transform[y][x] - transform_min) * (output_levels - 1) / transform_range;
        }
    }

    Image image_array = Image();
    char file_name_array[1000];
    sprintf(file_name_array, "output\\fourier_array_%ix%i.png", width, height);
    image_array.create_from_threshold_matrix(array);
    image_array.save(file_name_array);

    Image image_transform = Image();
    char file_name_transform[1000];
    sprintf(file_name_transform, "output\\fourier_transform_%ix%i.png", width, height);
    image_transform.create_from_threshold_matrix(transform);
    image_transform.save(file_name_transform);
    

    std::cout << "finished" << std::endl;
    return 0;
}

void error_diffusion_all(std::string file_name, Palette palette)
{
    std::string file_path_input = "input\\" + file_name + ".png";
    std::string file_path_output = "output\\" + file_name;
    Dither dither = Dither();
    dither.set_palette(palette);

    dither.load(file_path_input.c_str());
    dither.grayscale(GrayscaleMethod::BT709);
    dither.save((file_path_output + "_grayscale.png").c_str());

    dither.load(file_path_input.c_str());
    dither.reduce();
    dither.save((file_path_output + "_reduce.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::LINEAR, false);
    dither.save((file_path_output + "_linear_standard.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::LINEAR, true);
    dither.save((file_path_output + "_linear_alternate.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::FLOYD_STEINBERG, false);
    dither.save((file_path_output + "_floyd_steinberg_standard.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::FLOYD_STEINBERG, true);
    dither.save((file_path_output + "_floyd_steinberg_alternate.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::JARVICE_JUDICE_NINKE, false);
    dither.save((file_path_output + "_jarvis_judice_ninke_standard.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::JARVICE_JUDICE_NINKE, true);
    dither.save((file_path_output + "_jarvis_judice_ninke_alternate.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::STUCKI, false);
    dither.save((file_path_output + "_stucki_standard.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::STUCKI, true);
    dither.save((file_path_output + "_stucki_alternate.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::ATKINSON, false);
    dither.save((file_path_output + "_atkinson_standard.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::ATKINSON, true);
    dither.save((file_path_output + "_atkinson_alternate.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::BURKES, false);
    dither.save((file_path_output + "_burkes_standard.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::BURKES, true);
    dither.save((file_path_output + "_burkes_alternate.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::SIERRA, false);
    dither.save((file_path_output + "_sierra_standard.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::SIERRA, true);
    dither.save((file_path_output + "_sierra_alternate.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::SIERRA_TWO_ROW, false);
    dither.save((file_path_output + "_sierra_two_row_standard.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::SIERRA_TWO_ROW, true);
    dither.save((file_path_output + "_sierra_two_row_alternate.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::SIERRA_LITE, false);
    dither.save((file_path_output + "_sierra_lite_standard.png").c_str());

    dither.load(file_path_input.c_str());
    dither.error_diffusion(ErrorDiffusionAlgorithm::SIERRA_LITE, true);
    dither.save((file_path_output + "_sierra_lite_alternate.png").c_str());

    return;
}

void generate_bayer(int size, int output_levels)
{
    Bayer bayer = Bayer(size, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\bayer_%ix%i.png", size, size);

    bayer.generate_bayer_matrix();

    image.create_from_threshold_matrix(bayer.get_threshold_matrix());
    image.save(file_name);

    return;
}

void generate_bayer_all(int output_levels)
{
    clock_t time_start = clock();
    generate_bayer(2, output_levels);
    clock_t time_2x2 = clock();
    generate_bayer(4, output_levels);
    clock_t time_4x4 = clock();
    generate_bayer(8, output_levels);
    clock_t time_8x8 = clock();
    generate_bayer(16, output_levels);
    clock_t time_16x16 = clock();
    generate_bayer(32, output_levels);
    clock_t time_32x32 = clock();
    generate_bayer(64, output_levels);
    clock_t time_end = clock();

    std::cout << "Bayer:" << std::endl;
    std::cout << "2x2 time: " << 1000.0 * (time_2x2 - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "4x4 time: " << 1000.0 * (time_4x4 - time_2x2) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "8x8 time: " << 1000.0 * (time_8x8 - time_4x4) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "16x16 time: " << 1000.0 * (time_16x16 - time_8x8) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "32x32 time: " << 1000.0 * (time_32x32 - time_16x16) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "64x64 time: " << 1000.0 * (time_end - time_32x32) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "Total time: " << 1000.0 * (time_end - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;

    return;
}

void generate_blue_noise(int width, int height, double sigma, double coverage, int output_levels)
{
    BlueNoise blue_noise = BlueNoise(width, height, sigma, coverage, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\blue_noise_%ix%i_s%.1f.png", width, height, sigma);

    blue_noise.generate_blue_noise();

    image.create_from_threshold_matrix(blue_noise.get_threshold_matrix());
    image.save(file_name);

    return;
}

void generate_blue_noise_all(double sigma, double coverage, int output_levels)
{
    clock_t time_start = clock();
    generate_blue_noise(2, 2, sigma, std::max(coverage, 0.25), output_levels);
    clock_t time_2x2 = clock();
    generate_blue_noise(4, 4, sigma, std::max(coverage, 0.0625), output_levels);
    clock_t time_4x4 = clock();
    generate_blue_noise(8, 8, sigma, coverage, output_levels);
    clock_t time_8x8 = clock();
    generate_blue_noise(16, 16, sigma, coverage, output_levels);
    clock_t time_16x16 = clock();
    generate_blue_noise(32, 32, sigma, coverage, output_levels);
    clock_t time_32x32 = clock();
    generate_blue_noise(64, 64, sigma, coverage, output_levels);
    clock_t time_end = clock();

    std::cout << "Blue Noise:" << std::endl;
    std::cout << "2x2 time: " << 1000.0 * (time_2x2 - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "4x4 time: " << 1000.0 * (time_4x4 - time_2x2) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "8x8 time: " << 1000.0 * (time_8x8 - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "16x16 time: " << 1000.0 * (time_16x16 - time_8x8) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "32x32 time: " << 1000.0 * (time_32x32 - time_16x16) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "64x64 time: " << 1000.0 * (time_end - time_32x32) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "Total time: " << 1000.0 * (time_end - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;

    return;
}

void generate_brown_noise(int width, int height, double leaky_integrator, int output_levels)
{
    BrownNoise brown_noise = BrownNoise(width, height, leaky_integrator, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\brown_noise_%ix%i.png", width, height);

    brown_noise.generate_brown_noise();

    image.create_from_threshold_matrix(brown_noise.get_threshold_matrix());
    image.save(file_name);

    return;
}

void generate_brown_noise_all(double leaky_integrator, int output_levels)
{
    clock_t time_start = clock();
    generate_brown_noise(2, 2, leaky_integrator, output_levels);
    clock_t time_2x2 = clock();
    generate_brown_noise(4, 4, leaky_integrator, output_levels);
    clock_t time_4x4 = clock();
    generate_brown_noise(8, 8, leaky_integrator, output_levels);
    clock_t time_8x8 = clock();
    generate_brown_noise(16, 16, leaky_integrator, output_levels);
    clock_t time_16x16 = clock();
    generate_brown_noise(32, 32, leaky_integrator, output_levels);
    clock_t time_32x32 = clock();
    generate_brown_noise(64, 64, leaky_integrator, output_levels);
    clock_t time_end = clock();

    std::cout << "Brown Noise:" << std::endl;
    std::cout << "2x2 time: " << 1000.0 * (time_2x2 - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "4x4 time: " << 1000.0 * (time_4x4 - time_2x2) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "8x8 time: " << 1000.0 * (time_8x8 - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "16x16 time: " << 1000.0 * (time_16x16 - time_8x8) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "32x32 time: " << 1000.0 * (time_32x32 - time_16x16) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "64x64 time: " << 1000.0 * (time_end - time_32x32) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "Total time: " << 1000.0 * (time_end - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;

    return;
}

void generate_white_noise(int width, int height, int output_levels)
{
    WhiteNoise white_noise = WhiteNoise(width, height, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\white_noise_%ix%i.png", width, height);

    white_noise.generate_white_noise();

    image.create_from_threshold_matrix(white_noise.get_threshold_matrix());
    image.save(file_name);

    return;
}

void generate_white_noise_all(int output_levels)
{
    clock_t time_start = clock();
    generate_white_noise(2, 2, output_levels);
    clock_t time_2x2 = clock();
    generate_white_noise(4, 4, output_levels);
    clock_t time_4x4 = clock();
    generate_white_noise(8, 8, output_levels);
    clock_t time_8x8 = clock();
    generate_white_noise(16, 16, output_levels);
    clock_t time_16x16 = clock();
    generate_white_noise(32, 32, output_levels);
    clock_t time_32x32 = clock();
    generate_white_noise(64, 64, output_levels);
    clock_t time_end = clock();

    std::cout << "White Noise:" << std::endl;
    std::cout << "2x2 time: " << 1000.0 * (time_2x2 - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "4x4 time: " << 1000.0 * (time_4x4 - time_2x2) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "8x8 time: " << 1000.0 * (time_8x8 - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "16x16 time: " << 1000.0 * (time_16x16 - time_8x8) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "32x32 time: " << 1000.0 * (time_32x32 - time_16x16) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "64x64 time: " << 1000.0 * (time_end - time_32x32) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "Total time: " << 1000.0 * (time_end - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;

    return;
}
