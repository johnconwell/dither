#include "color.h"

// default constructor, initializes an all-black color
Color::Color()
{
    r = 0;
    g = 0;
    b = 0;
    a = Color::CHANNEL_MAX;
    return;
}

// initializes a color based on specified values
Color::Color(int16_t r, int16_t g, int16_t b, int16_t a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
    return;
}

// extracts a color from a hex code
Color::Color(uint32_t hex_code)
{
    r = (hex_code >> 24) & CHANNEL_MAX;
    g = (hex_code >> 16) & CHANNEL_MAX;
    b = (hex_code >> 8) & CHANNEL_MAX;
    a = (hex_code >> 0) & CHANNEL_MAX;
    return;
}

// returns true if the colors are equal, false otherwise
bool Color::operator==(const Color& other) const
{
    return (this->r == other.r) && (this->g == other.g) && (this->b == other.b) && (this->a == other.a);
}

// returns true if the lhs color is darker (closer to black) than the rhs color, false otherwise
bool Color::operator<(const Color& other) const
{
    return (this->r + this->g + this->b) < (other.r + other.g + other.b);
}

// returns this - other
// result may have channel values < 0
Color Color::operator-(const Color& other) const
{
    return Color(this->r - other.r, this->g - other.g, this->b - other.b, Color::CHANNEL_MAX);
}

// returns this + other
// result may have channel values > 255
Color Color::operator+(const Color& other) const
{
    return Color(this->r + other.r, this->g + other.g, this->b + other.b, Color::CHANNEL_MAX);
}

// returns this * scalar
Color Color::operator*(const double scalar) const
{
    return Color(uint16_t(this->r * scalar), uint16_t(this->g * scalar), uint16_t(this->b * scalar), Color::CHANNEL_MAX);
}

// returns the hex value of the color
uint32_t Color::hex()
{
    return (r << 24) + (g << 16) + (b << 8) + (a << 0);
}

// returns the euclidean distance squared to the specified color
int Color::distance_squared(Color other)
{
    return ((other.r - this->r) * (other.r - this->r)) + ((other.g - this->g) * (other.g - this->g)) + ((other.b - this->b) * (other.b - this->b));
}

// returns the euclidean distance to the specified color
double Color::distance(Color other)
{
    return sqrt(double(distance_squared(other)));
}

// returns a string representation of the color
std::string Color::to_string()
{
    std::stringstream ss;
    ss << std::hex << hex();
    std::string output = ss.str();
    return output;
}
