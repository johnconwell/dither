#include "color.h"

// default constructor initializes an all-black color
Color::Color()
{
    r = 0;
    g = 0;
    b = 0;
    a = 255;
}

Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}
