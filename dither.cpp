#include "dither.h"
#include <iostream>
#include <string>
// initializes empty image and palette
Dither::Dither()
{
    image = Image();
    palette = Palette();
    return;
}

// sets the palette
void Dither::set_palette(Palette palette)
{
    this->palette = palette;
    return;
}

// loads a png at the specified file path
unsigned int Dither::load(const char* file_name)
{
    return image.load(file_name);
}

// save a png to the specified file path
unsigned int Dither::save(const char* file_name)
{
    return image.save(file_name);
}

// converts image to grayscale using specified method
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

    return;
}

// changes each pixel in the image to the closest color in the palette
void Dither::reduce()
{
    for(int y = 0; y < image.get_height(); y++)
    {
        for(int x = 0; x < image.get_width(); x++)
        {
            Color color = image.get_pixel(x, y);
            Color palette_nearest = palette.nearest(color);
            image.set_pixel(palette_nearest, x, y);
        }
    }

    return;
}

// reduces the image to the colors in the palette and dithers using specified algorithm
// if alternate_direction is true, switches direction of error diffusion each row
void Dither::error_diffusion(ErrorDiffusionAlgorithm algorithm, bool alternate)
{
    if(alternate)
    {
        error_diffusion_alternate(algorithm);
    }
    else
    {
        error_diffusion_standard(algorithm);
    }

    return;
}

// dithers using specified algorithm, does not alternate direction on odd rows
void Dither::error_diffusion_standard(ErrorDiffusionAlgorithm algorithm)
{
    // initialize error containers
    Error error = Error(algorithm);
    std::vector<std::vector<std::vector<int>>> error_matrix(image.get_height(), std::vector<std::vector<int>>(image.get_width(), std::vector<int>(3, 0)));

    for(int y = 0; y < image.get_height(); y++)
    {
        for(int x = 0; x < image.get_width(); x++)
        {
            // set current pixel to nearest palette color (accounting for accumulated error)
            Color color = image.get_pixel(x, y);
            color.r += error_matrix[y][x][Color::INDEX_R];
            color.g += error_matrix[y][x][Color::INDEX_G];
            color.b += error_matrix[y][x][Color::INDEX_B];
            Color palette_nearest = palette.nearest(color);

            std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

            for(int index_error = 0; index_error < error.coordinates.size(); index_error++)
            {
                int new_x = x + error.coordinates[index_error].first;
                int new_y = y + error.coordinates[index_error].second;

                if(new_x < 0 || new_x >= image.get_width() || new_y < 0 || new_y >= image.get_height())
                {
                    continue;
                }

                error_matrix[new_y][new_x][Color::INDEX_R] += int(current_pixel_error[Color::INDEX_R] * error.scalars[error.coordinates[index_error]]);
                error_matrix[new_y][new_x][Color::INDEX_G] += int(current_pixel_error[Color::INDEX_G] * error.scalars[error.coordinates[index_error]]);
                error_matrix[new_y][new_x][Color::INDEX_B] += int(current_pixel_error[Color::INDEX_B] * error.scalars[error.coordinates[index_error]]);
            }

            image.set_pixel(palette_nearest, x, y);
        }
    }

    return;
}

// dithers using specified algorithm, alternates direction on odd rows
void Dither::error_diffusion_alternate(ErrorDiffusionAlgorithm algorithm)
{
    // initialize error containers
    Error error = Error(algorithm);
    std::vector<std::vector<std::vector<int>>> error_matrix(image.get_height(), std::vector<std::vector<int>>(image.get_width(), std::vector<int>(3, 0)));

    for(int y = 0; y < image.get_height(); y++)
    {
        // alternate direction on odd rows
        if(y % 2 == 1)
        {
            for(int x = image.get_width() - 1; x >= 0; x--)
            {
                // set current pixel to nearest palette color (accounting for accumulated error)
                Color color = image.get_pixel(x, y);
                color.r += error_matrix[y][x][Color::INDEX_R];
                color.g += error_matrix[y][x][Color::INDEX_G];
                color.b += error_matrix[y][x][Color::INDEX_B];
                Color palette_nearest = palette.nearest(color);

                std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

                for(int index_error = 0; index_error < error.coordinates.size(); index_error++)
                {
                    int new_x = x - error.coordinates[index_error].first; // flip x when we are going backwards
                    int new_y = y + error.coordinates[index_error].second;

                    if(new_x < 0 || new_x >= image.get_width() || new_y < 0 || new_y >= image.get_height())
                    {
                        continue;
                    }

                    error_matrix[new_y][new_x][Color::INDEX_R] += int(current_pixel_error[Color::INDEX_R] * error.scalars[error.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_G] += int(current_pixel_error[Color::INDEX_G] * error.scalars[error.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_B] += int(current_pixel_error[Color::INDEX_B] * error.scalars[error.coordinates[index_error]]);
                }

                image.set_pixel(palette_nearest, x, y);
            }
        }
        else
        {
            for(int x = 0; x < image.get_width(); x++)
            {
                // set current pixel to nearest palette color (accounting for accumulated error)
                Color color = image.get_pixel(x, y);
                color.r += error_matrix[y][x][Color::INDEX_R];
                color.g += error_matrix[y][x][Color::INDEX_G];
                color.b += error_matrix[y][x][Color::INDEX_B];
                Color palette_nearest = palette.nearest(color);

                std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

                for(int index_error = 0; index_error < error.coordinates.size(); index_error++)
                {
                    int new_x = x + error.coordinates[index_error].first;
                    int new_y = y + error.coordinates[index_error].second;

                    if(new_x < 0 || new_x >= image.get_width() || new_y < 0 || new_y >= image.get_height())
                    {
                        continue;
                    }

                    error_matrix[new_y][new_x][Color::INDEX_R] += int(current_pixel_error[Color::INDEX_R] * error.scalars[error.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_G] += int(current_pixel_error[Color::INDEX_G] * error.scalars[error.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_B] += int(current_pixel_error[Color::INDEX_B] * error.scalars[error.coordinates[index_error]]);
                }

                image.set_pixel(palette_nearest, x, y);
            }
        }
    }

    return;
}
