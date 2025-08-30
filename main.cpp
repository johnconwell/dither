#include "blue_noise.h"
#include "dither.h"
#include "error.h"
#include "grayscale.h"
#include "ordered.h"
#include "palette.h"
#include <iostream>
#include <string>
#include <time.h>
#include <vector>

void error_diffusion_all(std::string file_name, Palette palette);
void generate_bayer_matrixes();
void generate_blue_noise(int width, int height, double sigma, double coverage, int output_levels);

int main()
{
    srand(time(NULL));
    Palette palette_black_white = Palette("BLACK_WHITE", Palette::preset_palettes.at(PresetPalette::BLACK_WHITE));
    Palette palette_1bit_monitor_glow = Palette("_1BIT_MONITOR_GLOW", Palette::preset_palettes.at(PresetPalette::_1BIT_MONITOR_GLOW));
    Palette palette_twilight5 = Palette("_1BIT_MONITOR_GLOW", Palette::preset_palettes.at(PresetPalette::_1BIT_MONITOR_GLOW));

    // int size = 16;
    // error_diffusion_all("statue", palette_black_white);
    // Ordered ordered = Ordered();
    // ordered.white_noise(size);
    // std::cout << ordered.to_string() << std::endl;

    // generate_bayer_matrixes();

    double sigma = 1.9;
    double coverage = 0.1;
    int output_levels = Color::CHANNEL_MAX + 1;

    clock_t time_start = clock();

    generate_blue_noise(8, 8, sigma, coverage, output_levels);

    clock_t time_8x8 = clock();

    generate_blue_noise(16, 16, sigma, coverage, output_levels);

    clock_t time_16x16 = clock();

    generate_blue_noise(32, 32, sigma, coverage, output_levels);

    clock_t time_32x32 = clock();

    generate_blue_noise(64, 64, sigma, coverage, output_levels);

    clock_t time_64x64 = clock();

    generate_blue_noise(128, 128, sigma, coverage, output_levels);

    clock_t time_128x128 = clock();

    generate_blue_noise(256, 256, sigma, coverage, output_levels);

    clock_t time_end = clock();

    std::cout << "Total time: " << 1000.0 * (time_end - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "8x8 time: " << 1000.0 * (time_8x8 - time_start) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "16x16 time: " << 1000.0 * (time_16x16 - time_8x8) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "32x32 time: " << 1000.0 * (time_32x32 - time_16x16) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "64x64 time: " << 1000.0 * (time_64x64 - time_32x32) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "128x128 time: " << 1000.0 * (time_128x128 - time_64x64) / CLOCKS_PER_SEC << " ms" << std::endl;
    std::cout << "256x256 time: " << 1000.0 * (time_end - time_128x128) / CLOCKS_PER_SEC << " ms" << std::endl;

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

void generate_bayer_matrixes()
{
    Image image = Image();
    Ordered ordered = Ordered();

    ordered.bayer_matrix(2, Color::CHANNEL_MAX + 1);
    image.create_from_threshold_matrix(ordered.threshold_matrix);
    image.save("output\\bayer_2x2.png");

    ordered.bayer_matrix(4, Color::CHANNEL_MAX + 1);
    image.create_from_threshold_matrix(ordered.threshold_matrix);
    image.save("output\\bayer_4x4.png");

    ordered.bayer_matrix(8, Color::CHANNEL_MAX + 1);
    image.create_from_threshold_matrix(ordered.threshold_matrix);
    image.save("output\\bayer_8x8.png");

    ordered.bayer_matrix(16, Color::CHANNEL_MAX + 1);
    image.create_from_threshold_matrix(ordered.threshold_matrix);
    image.save("output\\bayer_16x16.png");

    ordered.bayer_matrix(32, Color::CHANNEL_MAX + 1);
    image.create_from_threshold_matrix(ordered.threshold_matrix);
    image.save("output\\bayer_32x32.png");

    return;
}

void generate_blue_noise(int width, int height, double sigma, double coverage, int output_levels)
{
    BlueNoise blue_noise = BlueNoise(width, height, sigma, coverage, output_levels);
    Image image = Image();
    std::string file_name = "output\\blue_noise_" + std::to_string(width) + "x" + std::to_string(height) + "_s" + std::to_string(sigma) + ".png";

    blue_noise.generate_dither_array();

    image.create_from_threshold_matrix(blue_noise.get_dither_array());
    image.save(file_name.c_str());

    return;
}
