#ifndef __COLOR_H
#define __COLOR_H

#include <algorithm> // std::clamp
#include <array> // std::array
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

    void to_grayscale(std::vector<double> grayscale_weights);

    std::string to_string();
    std::string to_string_hex();

    // widen a {min, max} colour range so that it includes this colour (each channel independently)
    void expand_range(std::vector<Color>& range) const;

    int r;
    int g;
    int b;
    int a;
};

// Working colour for the dither maths (Phase 3): float channels on the 0-255 scale, either
// sRGB-encoded or linear light. Keeping the working values in float means diffused error,
// distances and linear values are never truncated to 8 bits (B3, Q1, Q6).
struct RGBf
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    RGBf() = default;
    RGBf(float r, float g, float b) : r(r), g(g), b(b) {}
    explicit RGBf(const Color& c) : r(static_cast<float>(c.r)), g(static_cast<float>(c.g)), b(static_cast<float>(c.b)) {}

    RGBf operator+(const RGBf& o) const { return {r + o.r, g + o.g, b + o.b}; }
    RGBf operator-(const RGBf& o) const { return {r - o.r, g - o.g, b - o.b}; }
    RGBf operator*(float s) const { return {r * s, g * s, b * s}; }
    RGBf& operator+=(const RGBf& o) { r += o.r; g += o.g; b += o.b; return *this; }

    RGBf clamped() const;
    // HSL lightness, 0-1
    double lightness() const;

    // BT.709-weighted squared Euclidean distance (the EUCLIDEAN_DISTANCE mapping method)
    double distance_euclidean_squared(const RGBf& o) const;
    double distance_euclidean(const RGBf& o) const;
    // unweighted L1 distance (the MANHATTAN_DISTANCE mapping method)
    double distance_manhattan(const RGBf& o) const;
};

// Converts between 8-bit encoded values and linear light (both on the 0-255 scale).
// file_gamma <= 0 selects the standard sRGB curve; a positive value is the decoding
// exponent from a PNG gAMA chunk (for example 2.2), used as a pure power curve.
class Transfer
{
public:
    explicit Transfer(double file_gamma = 0.0);

    float to_linear(int encoded) const { return lut[static_cast<std::size_t>(std::clamp(encoded, 0, static_cast<int>(Color::CHANNEL_MAX)))]; }
    RGBf to_linear(const Color& c) const { return {to_linear(c.r), to_linear(c.g), to_linear(c.b)}; }
    // rounds to the nearest 8-bit encoded value
    int to_encoded(float linear) const;
    Color to_encoded(const RGBf& c, int alpha) const { return Color(to_encoded(c.r), to_encoded(c.g), to_encoded(c.b), alpha); }

private:
    double gamma;
    std::array<float, Color::CHANNEL_VALUES> lut;
};

#endif
