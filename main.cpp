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

    BlueNoise blue_noise = BlueNoise(32, 32, 0.4, 0.1);

    blue_noise.generate_initial_binary_pattern();

    std::cout << "intiial binary pattern:" << std::endl;
    std::cout << blue_noise.to_string_binary_pattern_initial() << std::endl;

    blue_noise.generate_dither_array_phase_1();

    std::cout << "prototype binary pattern 1:" << std::endl;
    std::cout << blue_noise.to_string_binary_pattern_prototype() << std::endl;
    std::cout << "dither array 1:" << std::endl;
    std::cout << blue_noise.to_string_dither_array() << std::endl;

    blue_noise.generate_dither_array_phase_2();

    std::cout << "prototype binary pattern 2:" << std::endl;
    std::cout << blue_noise.to_string_binary_pattern_prototype() << std::endl;
    std::cout << "dither array 2:" << std::endl;
    std::cout << blue_noise.to_string_dither_array() << std::endl;

    blue_noise.generate_dither_array_phase_3();

    std::cout << "prototype binary pattern 3:" << std::endl;
    std::cout << blue_noise.to_string_binary_pattern_prototype() << std::endl;
    std::cout << "dither array 3:" << std::endl;
    std::cout << blue_noise.to_string_dither_array() << std::endl;

    blue_noise.normalize_dither_array(Color::CHANNEL_MAX + 1);
    std::cout << "dither array 4:" << std::endl;
    std::cout << blue_noise.to_string_dither_array() << std::endl;

    Image image = Image();
    image.create_from_threshold_matrix(blue_noise.get_dither_array());
    image.save("output\\blue_noise_32x32.png");

    generate_bayer_matrixes();

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
}
