#include "dither.h"

Dither::Dither()
{
    image = Image();
}

unsigned int Dither::load(const char* file_name)
{
    return image.load(file_name);
}

unsigned int Dither::save(const char* file_name)
{
    return image.save(file_name);
}

void Dither::grayscale(GrayscaleMethod method)
{
    for(int y = 0; y < image.get_height(); y++)
    {
        for(int x = 0; x < image.get_width(); x++)
        {
            Color color = image.get_pixel(x, y);
            uint8_t channel_value = Grayscale::channel_value(color, method);
            Color color_grayscale = Color(channel_value, channel_value, channel_value, Color::CHANNEL_MAX);
            image.set_pixel(color_grayscale, x, y);
        }
    }
}


