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

// fills image with a grayscale representation of specified matrix
void Image::create_from_matrix(std::vector<std::vector<int>> matrix)
{
    height = matrix.size();
    width = matrix[0].size();
    pixels.resize(width * height * Color::NUM_BYTES_COLOR);

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            unsigned char channel_value = static_cast<unsigned char>(std::clamp(matrix[y][x], 0, Color::CHANNEL_MAX));
            int index_pixels = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
            pixels[index_pixels + 0] = channel_value;
            pixels[index_pixels + 1] = channel_value;
            pixels[index_pixels + 2] = channel_value;
            pixels[index_pixels + 3] = Color::CHANNEL_MAX;
        }
    }

    return;
}

// creates a threshold matrix from the loaded image, assuming image is grayscale
std::vector<std::vector<int>> Image::get_matrix_from_image()
{
    std::vector<std::vector<int>> matrix = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            matrix[y][x] = pixels[y * Color::NUM_BYTES_COLOR + x];
        }
    }

    return matrix;
}
