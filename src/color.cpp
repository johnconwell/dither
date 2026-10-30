#include "color.h"

Color::Color()
{
    r = 0;
    g = 0;
    b = 0;
    a = CHANNEL_MAX;
    return;
}

Color::Color(int r, int g, int b, int a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
    return;
}

Color::Color(unsigned int hex_code)
{
    if(hex_code > 0xffffff)
    {
        std::cerr << "Error: color hex code invalid - " << hex_code << std::endl;
        std::exit(EXIT_FAILURE);
    }

    r = (hex_code >> 16) & CHANNEL_MAX;
    g = (hex_code >> 8) & CHANNEL_MAX;
    b = (hex_code >> 0) & CHANNEL_MAX;
    a = CHANNEL_MAX;
    
    return;
}

unsigned int Color::get_hex()
{
    return (r << 16) + (g << 8) + (b << 0);
}

double Color::get_lightness()
{
    return static_cast<double>(std::max(r, std::max(g, b)) + std::min(r, std::min(g, b))) / (2.0 * static_cast<double>(CHANNEL_MAX));
}

bool Color::equals(const Color& other) const
{
    return (this->r == other.r) && (this->g == other.g) && (this->b == other.b) && (this->a == other.a);
}

Color Color::add(const Color& other) const
{
    return Color(this->r + other.r, this->g + other.g, this->b + other.b, std::max(this->a, other.a));
}

int Color::distance_euclidean_squared(Color other, std::vector<double> channel_weights)
{
    return (channel_weights[INDEX_R] * (other.r - this->r) * (other.r - this->r)) + (channel_weights[INDEX_G] * (other.g - this->g) * (other.g - this->g)) + (channel_weights[INDEX_B] * (other.b - this->b) * (other.b - this->b));
}

double Color::distance_euclidean(Color other)
{
    return sqrt(static_cast<double>(distance_euclidean_squared(other)));
}

int Color::distance_manhattan(Color other)
{
    return abs(other.r - this->r) + abs(other.g - this->g) + abs(other.b - this->b);
}

double Color::grayscale_value(std::vector<double> grayscale_weights)
{
    return static_cast<double>(r) * grayscale_weights[INDEX_R] + static_cast<double>(g) * grayscale_weights[INDEX_G] + static_cast<double>(b) * grayscale_weights[INDEX_B];
}

void Color::to_grayscale(std::vector<double> grayscale_weights)
{
    double value = grayscale_value(grayscale_weights);
    value = std::clamp(value, 0.0, static_cast<double>(CHANNEL_MAX));
    r = static_cast<int>(value);
    g = static_cast<int>(value);
    b = static_cast<int>(value);
    return;
}

std::string Color::to_string()
{
    std::string output = "";
    output += std::to_string(r) + " " + std::to_string(g) + " " + std::to_string(b);
    return output;
}

std::string Color::to_string_hex()
{
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(6) << std::hex << get_hex();
    std::string output = ss.str();
    return output;
}

void Color::expand_range(std::vector<Color>& range) const
{
    const std::size_t INDEX_COLOR_MIN = 0;
    const std::size_t INDEX_COLOR_MAX = 1;

    // independent ifs (not else-if): the first value seen must be able to set both the min and the max (B4)
    range[INDEX_COLOR_MIN].r = std::min(range[INDEX_COLOR_MIN].r, r);
    range[INDEX_COLOR_MAX].r = std::max(range[INDEX_COLOR_MAX].r, r);
    range[INDEX_COLOR_MIN].g = std::min(range[INDEX_COLOR_MIN].g, g);
    range[INDEX_COLOR_MAX].g = std::max(range[INDEX_COLOR_MAX].g, g);
    range[INDEX_COLOR_MIN].b = std::min(range[INDEX_COLOR_MIN].b, b);
    range[INDEX_COLOR_MAX].b = std::max(range[INDEX_COLOR_MAX].b, b);

    return;
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
