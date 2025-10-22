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

void Color::to_linear(double gamma)
{
    const double r_scaled = static_cast<double>(r) / static_cast<double>(CHANNEL_VALUES);
    const double g_scaled = static_cast<double>(g) / static_cast<double>(CHANNEL_VALUES);
    const double b_scaled = static_cast<double>(b) / static_cast<double>(CHANNEL_VALUES);

    const double r_linear = (r_scaled < 0.04045) ? (r_scaled / 12.92) : pow((r_scaled + 0.055) / 1.055, gamma);
    const double g_linear = (g_scaled < 0.04045) ? (g_scaled / 12.92) : pow((g_scaled + 0.055) / 1.055, gamma);
    const double b_linear = (b_scaled < 0.04045) ? (b_scaled / 12.92) : pow((b_scaled + 0.055) / 1.055, gamma);

    r = static_cast<int>(static_cast<double>(CHANNEL_VALUES) * r_linear);
    g = static_cast<int>(static_cast<double>(CHANNEL_VALUES) * g_linear);
    b = static_cast<int>(static_cast<double>(CHANNEL_VALUES) * b_linear);

    return;
}

void Color::to_srgb(double gamma)
{
    const double r_scaled = static_cast<double>(r) / static_cast<double>(CHANNEL_VALUES);
    const double g_scaled = static_cast<double>(g) / static_cast<double>(CHANNEL_VALUES);
    const double b_scaled = static_cast<double>(b) / static_cast<double>(CHANNEL_VALUES);

    const double r_linear = (r_scaled < 0.0031308) ? (r_scaled * 12.92) : (1.055 * pow(r_scaled, 1.0 / gamma) - 0.055);
    const double g_linear = (g_scaled < 0.0031308) ? (g_scaled * 12.92) : (1.055 * pow(g_scaled, 1.0 / gamma) - 0.055);
    const double b_linear = (b_scaled < 0.0031308) ? (b_scaled * 12.92) : (1.055 * pow(b_scaled, 1.0 / gamma) - 0.055);

    r = static_cast<int>(static_cast<double>(CHANNEL_VALUES) * r_linear);
    g = static_cast<int>(static_cast<double>(CHANNEL_VALUES) * g_linear);
    b = static_cast<int>(static_cast<double>(CHANNEL_VALUES) * b_linear);

    return;
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
