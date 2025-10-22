#ifndef __COLOR_H
#define __COLOR_H

#include <algorithm> // std::clamp
#include <cmath> // sqrt()
#include <cstdlib> // std::exit
#include <iomanip> // std::hex
#include <iostream> // std::cerr, std::endl
#include <sstream> // std::stringstream
#include <string> // std::string
#include <unordered_map> // std::unordered_map
#include <vector> // std::vector

class Color
{
public:
    static inline constexpr std::size_t NUM_BYTES = 4;
    static inline constexpr std::size_t INDEX_R = 0;
    static inline constexpr std::size_t INDEX_G = 1;
    static inline constexpr std::size_t INDEX_B = 2;
    static inline constexpr std::size_t INDEX_A = 3;
    static inline constexpr std::size_t CHANNEL_MAX = 255;
    static inline constexpr std::size_t CHANNEL_VALUES = 256;
    static inline constexpr std::size_t HEX_MAX = 0xffffff;

    Color();
    Color(int r, int g, int b, int a = CHANNEL_MAX);
    Color(unsigned int hex_code);

    unsigned int get_hex();
    double get_lightness();

    bool equals(const Color& other) const;
    Color add(const Color& other) const;

    int distance_euclidean_squared(Color other, std::vector<double> channel_weights = {0.2126, 0.7152, 0.0722});
    double distance_euclidean(Color other);
    int distance_manhattan(Color other);

    double grayscale_value(std::vector<double> grayscale_weights);

    void to_linear(double gamma);
    void to_srgb(double gamma);
    void to_grayscale(std::vector<double> grayscale_weights);

    std::string to_string();
    std::string to_string_hex();

    int r;
    int g;
    int b;
    int a;
};

#endif
