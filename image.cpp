#include "image.h"
#include <iostream>

// initializes empty image
Image::Image()
{
    pixels.resize(0);
    width = 0;
    height = 0;
    return;
}

// erases all image data and reinitializes object
void Image::clear()
{
    pixels.resize(0);
    width = 0;
    height = 0;
    return;
}

// returns image's width
unsigned int Image::get_width()
{
    return width;
}

// returns image's height
unsigned int Image::get_height()
{
    return height;
}

// loads a png from the specified path
unsigned int Image::load(const char* file_name)
{
    clear();
    
    unsigned error = lodepng::decode(pixels, width, height, file_name);

    if(error)
    {
        std::cout << "error: load - " << error << ": " << lodepng_error_text(error) << std::endl;
    }

    return error;
}

// saves a png to the specified path
unsigned int Image::save(const char* file_name)
{
    unsigned error = lodepng::encode(file_name, pixels, width, height);

    if(error)
    {
        std::cout << "error: save - " << error << ": "<< lodepng_error_text(error) << std::endl;
    }

    return error;
}

// gets the pixel color at (x, y)
Color Image::get_pixel(int x, int y)
{
    Color color = Color();
    int index_start = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
    color.r = pixels[index_start + Color::INDEX_R];
    color.g = pixels[index_start + Color::INDEX_G];
    color.b = pixels[index_start + Color::INDEX_B];
    color.a = pixels[index_start + Color::INDEX_A];
    return color;
}

// sets the pixel color at (x, y)
void Image::set_pixel(Color color, int x, int y)
{
    int index_start = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
    pixels[index_start + Color::INDEX_R] = color.r;
    pixels[index_start + Color::INDEX_G] = color.g;
    pixels[index_start + Color::INDEX_B] = color.b;
    pixels[index_start + Color::INDEX_A] = color.a;
    return;
}
