#include "grayscale.h"

const std::unordered_map<GrayscaleMethod, const double*> Grayscale::weights = {
    {GrayscaleMethod::STANDARD, Grayscale::STANDARD},
    {GrayscaleMethod::BT709, Grayscale::BT709},
    {GrayscaleMethod::CHANNEL_R, Grayscale::CHANNEL_R},
    {GrayscaleMethod::CHANNEL_G, Grayscale::CHANNEL_G},
    {GrayscaleMethod::CHANNEL_B, Grayscale::CHANNEL_B},
    {GrayscaleMethod::CHANNEL_RG, Grayscale::CHANNEL_RG},
    {GrayscaleMethod::CHANNEL_RB, Grayscale::CHANNEL_RB},
    {GrayscaleMethod::CHANNEL_GB, Grayscale::CHANNEL_GB}
};

// returns single channel grayscale value of color using specified grayscale method
int16_t Grayscale::channel_value(Color color, GrayscaleMethod method)
{
    const double* grayscale_weights = Grayscale::weights.at(method);
    double component_r = double(color.r) * grayscale_weights[Color::INDEX_R];
    double component_g = double(color.g) * grayscale_weights[Color::INDEX_G];
    double component_b = double(color.b) * grayscale_weights[Color::INDEX_B];
    return int16_t(component_r + component_g + component_b);
}