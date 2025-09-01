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
size_t Image::get_width()
{
    return width;
}

// returns image's height
size_t Image::get_height()
{
    return height;
}

// loads a png from the specified path
size_t Image::load(const char* file_name)
{
    clear();
    
    size_t error = lodepng::decode(pixels, width, height, file_name);

    if(error)
    {
        std::cout << "error: load - " << error << ": " << lodepng_error_text(error) << std::endl;
    }

    return error;
}

// saves a png to the specified path
size_t Image::save(const char* file_name)
{
    size_t error = lodepng::encode(file_name, pixels, width, height);

    if(error)
    {
        std::cout << "error: save - " << error << ": "<< lodepng_error_text(error) << std::endl;
    }

    return error;
}

// gets the pixel color at (x, y)
Color Image::get_pixel(unsigned int x, unsigned int y)
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
void Image::set_pixel(Color color, unsigned int x, unsigned int y)
{
    int index_start = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
    pixels[index_start + Color::INDEX_R] = color.r;
    pixels[index_start + Color::INDEX_G] = color.g;
    pixels[index_start + Color::INDEX_B] = color.b;
    pixels[index_start + Color::INDEX_A] = color.a;
    return;
}

// returns a width x height sized array where each cell is the gradient of the corresponding pixel on the image
// gradient is the difference between the current pixel and the average of neighbors
std::vector<std::vector<int>> Image::gradient_map()
{
    std::vector<std::vector<int>> gm(width, std::vector<int>(height, 0));

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            
        }
    }

    return gm;
}

// fills image with a grayscale representation of specified threshold map
void Image::create_from_threshold_matrix(std::vector<std::vector<int>> threshold_matrix)
{
    height = threshold_matrix.size();
    width = threshold_matrix[0].size();
    pixels.resize(width * height * 4);

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            int index_pixels = 4 * width * y + 4 * x;
            pixels[index_pixels + 0] = threshold_matrix[y][x];
            pixels[index_pixels + 1] = threshold_matrix[y][x];
            pixels[index_pixels + 2] = threshold_matrix[y][x];
            pixels[index_pixels + 3] = Color::CHANNEL_MAX;
        }
    }

    return;
}
