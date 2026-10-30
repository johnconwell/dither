#include "color.h"
#include "error.h"
#include <cmath> // std::sqrt, std::pow, std::fabs, std::lround
#include <iomanip> // std::setw, std::setfill, std::hex
#include <sstream> // std::stringstream

Color::Color(unsigned int hex_code)
{
    if(hex_code > HEX_MAX)
    {
        throw DitherError("color hex code invalid - " + std::to_string(hex_code));
    }

    r = static_cast<int>((hex_code >> 16) & 0xffu);
    g = static_cast<int>((hex_code >> 8) & 0xffu);
    b = static_cast<int>(hex_code & 0xffu);
    a = CHANNEL_MAX;
}

unsigned int Color::get_hex() const
{
    return (static_cast<unsigned int>(r) << 16) + (static_cast<unsigned int>(g) << 8) + static_cast<unsigned int>(b);
}

void Color::to_grayscale(const std::array<double, 3>& grayscale_weights)
{
    double value = static_cast<double>(r) * grayscale_weights[INDEX_R] + static_cast<double>(g) * grayscale_weights[INDEX_G] + static_cast<double>(b) * grayscale_weights[INDEX_B];
    value = std::clamp(value, 0.0, static_cast<double>(CHANNEL_MAX));
    r = static_cast<int>(value);
    g = static_cast<int>(value);
    b = static_cast<int>(value);
}

std::string Color::to_string() const
{
    return std::to_string(r) + " " + std::to_string(g) + " " + std::to_string(b);
}

std::string Color::to_string_hex() const
{
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(6) << std::hex << get_hex();
    return ss.str();
}

RGBf RGBf::clamped() const
{
    constexpr float MAX = static_cast<float>(Color::CHANNEL_MAX);
    return {std::clamp(r, 0.0f, MAX), std::clamp(g, 0.0f, MAX), std::clamp(b, 0.0f, MAX)};
}

double RGBf::lightness() const
{
    return (static_cast<double>(std::max({r, g, b})) + static_cast<double>(std::min({r, g, b}))) / (2.0 * static_cast<double>(Color::CHANNEL_MAX));
}

double RGBf::distance_euclidean_squared(const RGBf& o) const
{
    const double dr = static_cast<double>(o.r) - r;
    const double dg = static_cast<double>(o.g) - g;
    const double db = static_cast<double>(o.b) - b;
    return 0.2126 * dr * dr + 0.7152 * dg * dg + 0.0722 * db * db;
}

double RGBf::distance_euclidean(const RGBf& o) const
{
    return std::sqrt(distance_euclidean_squared(o));
}

double RGBf::distance_manhattan(const RGBf& o) const
{
    return std::fabs(static_cast<double>(o.r) - r) + std::fabs(static_cast<double>(o.g) - g) + std::fabs(static_cast<double>(o.b) - b);
}

namespace
{
    // standard sRGB transfer function, on 0-1 values
    double srgb_decode(double v) { return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); }
    double srgb_encode(double v) { return v <= 0.0031308 ? v * 12.92 : 1.055 * std::pow(v, 1.0 / 2.4) - 0.055; }
}

Transfer::Transfer(double file_gamma) : gamma(file_gamma), lut{}
{
    const double MAX = static_cast<double>(Color::CHANNEL_MAX);

    for(std::size_t i = 0; i < Color::CHANNEL_VALUES; i++)
    {
        const double v = static_cast<double>(i) / MAX;
        lut[i] = static_cast<float>(MAX * (gamma > 0.0 ? std::pow(v, gamma) : srgb_decode(v)));
    }
}

int Transfer::to_encoded(float linear) const
{
    const double MAX = static_cast<double>(Color::CHANNEL_MAX);
    const double v = std::clamp(static_cast<double>(linear) / MAX, 0.0, 1.0);
    const double encoded = gamma > 0.0 ? std::pow(v, 1.0 / gamma) : srgb_encode(v);
    return static_cast<int>(std::lround(encoded * MAX));
}
