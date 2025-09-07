#include "dither.h"
#include <iostream>

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
size_t Dither::load(const char* file_name)
{
    return image.load(file_name);
}

// save a png to the specified file path
size_t Dither::save(const char* file_name)
{
    return image.save(file_name);
}

// converts image to grayscale using specified method
void Dither::grayscale(GrayscaleMethod method)
{
    size_t height = image.get_height();
    size_t width = image.get_width();
    
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            Color color = image.get_pixel(x, y);
            int16_t channel_value = Grayscale::channel_value(color, method);
            Color color_grayscale = Color(channel_value, channel_value, channel_value, Color::CHANNEL_MAX);
            image.set_pixel(color_grayscale, x, y);
        }
    }

    return;
}

// changes each pixel in the image to the closest color in the palette
void Dither::reduce()
{
    size_t height = image.get_height();
    size_t width = image.get_width();
    
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
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

// reduces the image to the colors in the palette and dithers against the specified threshold matrix
void Dither::ordered(std::vector<std::vector<int>> threshold_matrix)
{
    size_t image_height = image.get_height();
    size_t image_width = image.get_width();
    size_t palette_size = palette.size();
    size_t threshold_matrix_height = threshold_matrix.size();
    size_t threshold_matrix_width = threshold_matrix[0].size();

    Color color;
    size_t index_palette_nearest_lower;
    Color palette_nearest_lower;
    Color palette_nearest_upper;
    int threshold_value;
    int threshold_value_scaled;

    palette.sort();

    for(size_t y = 0; y < image_height; y++)
    {
        for(size_t x = 0; x < image_width; x++)
        {
            // std::cout << y << " " << x << std::endl;
            color = image.get_pixel(x, y);
            // std::cout << "here1" << std::endl;
            index_palette_nearest_lower = palette.nearest_index_lower(color);
            // std::cout << index_palette_nearest_lower << std::endl;
            palette_nearest_lower = palette.get_color_at(index_palette_nearest_lower);
            // std::cout << "here3" << std::endl;
            palette_nearest_upper = palette.get_color_at(index_palette_nearest_lower + 1);
            // std::cout << "here4" << std::endl;
            threshold_value = threshold_matrix[y % threshold_matrix_height][x % threshold_matrix_width];
            threshold_value_scaled = palette_nearest_lower.r + (threshold_value / palette_size);
            // std::cout << index_palette_nearest_lower << std::endl;
            if(color.r < threshold_value_scaled)
            {
                image.set_pixel(palette_nearest_lower, x, y);
            }
            else
            {
                image.set_pixel(palette_nearest_upper, x, y);
            }
        }
    }

    return;
}

// dithers using specified algorithm, does not alternate direction on odd rows
void Dither::error_diffusion_standard(ErrorDiffusionAlgorithm algorithm)
{
    // initialize error diffusion containers
    ErrorDiffusion error_diffusion = ErrorDiffusion(algorithm);
    std::vector<std::vector<std::vector<int>>> error_matrix(image.get_height(), std::vector<std::vector<int>>(image.get_width(), std::vector<int>(3, 0)));
    size_t height = image.get_height();
    size_t width = image.get_width();
    
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            // set current pixel to nearest palette color (accounting for accumulated error)
            Color color = image.get_pixel(x, y);
            color.r += error_matrix[y][x][Color::INDEX_R];
            color.g += error_matrix[y][x][Color::INDEX_G];
            color.b += error_matrix[y][x][Color::INDEX_B];
            Color palette_nearest = palette.nearest(color);

            std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

            for(size_t index_error = 0; index_error < error_diffusion.coordinates.size(); index_error++)
            {
                size_t new_x = x + error_diffusion.coordinates[index_error].first;
                size_t new_y = y + error_diffusion.coordinates[index_error].second;

                if(new_x < 0 || new_x >= width || new_y < 0 || new_y >= height)
                {
                    continue;
                }

                error_matrix[new_y][new_x][Color::INDEX_R] += static_cast<int>(current_pixel_error[Color::INDEX_R] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                error_matrix[new_y][new_x][Color::INDEX_G] += static_cast<int>(current_pixel_error[Color::INDEX_G] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                error_matrix[new_y][new_x][Color::INDEX_B] += static_cast<int>(current_pixel_error[Color::INDEX_B] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
            }

            image.set_pixel(palette_nearest, x, y);
        }
    }

    return;
}

// dithers using specified algorithm, alternates direction on odd rows
void Dither::error_diffusion_alternate(ErrorDiffusionAlgorithm algorithm)
{
    // initialize error diffusion containers
    ErrorDiffusion error_diffusion = ErrorDiffusion(algorithm);
    std::vector<std::vector<std::vector<int>>> error_matrix(image.get_height(), std::vector<std::vector<int>>(image.get_width(), std::vector<int>(Color::NUM_BYTES_COLOR - 1, 0))); // RGB values (no A)
    size_t height = image.get_height();
    size_t width = image.get_width();

    for(size_t y = 0; y < height; y++)
    {
        // alternate direction on odd rows
        if(y % 2 == 1)
        {
            for(int x = width - 1; x >= 0; x--)
            {
                // set current pixel to nearest palette color (accounting for accumulated error)
                Color color = image.get_pixel(x, y);
                color.r += error_matrix[y][x][Color::INDEX_R];
                color.g += error_matrix[y][x][Color::INDEX_G];
                color.b += error_matrix[y][x][Color::INDEX_B];
                Color palette_nearest = palette.nearest(color);

                std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

                for(size_t index_error = 0; index_error < error_diffusion.coordinates.size(); index_error++)
                {
                    size_t new_x = x - error_diffusion.coordinates[index_error].first; // flip x when we are going backwards
                    size_t new_y = y + error_diffusion.coordinates[index_error].second;

                    if(new_x < 0 || new_x >= width || new_y < 0 || new_y >= height)
                    {
                        continue;
                    }

                    error_matrix[new_y][new_x][Color::INDEX_R] += static_cast<int>(current_pixel_error[Color::INDEX_R] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_G] += static_cast<int>(current_pixel_error[Color::INDEX_G] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_B] += static_cast<int>(current_pixel_error[Color::INDEX_B] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                }

                image.set_pixel(palette_nearest, x, y);
            }
        }
        else
        {
            for(size_t x = 0; x < width; x++)
            {
                // set current pixel to nearest palette color (accounting for accumulated error)
                Color color = image.get_pixel(x, y);
                color.r += error_matrix[y][x][Color::INDEX_R];
                color.g += error_matrix[y][x][Color::INDEX_G];
                color.b += error_matrix[y][x][Color::INDEX_B];
                Color palette_nearest = palette.nearest(color);

                std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

                for(size_t index_error = 0; index_error < error_diffusion.coordinates.size(); index_error++)
                {
                    size_t new_x = x + error_diffusion.coordinates[index_error].first;
                    size_t new_y = y + error_diffusion.coordinates[index_error].second;

                    if(new_x < 0 || new_x >= width || new_y < 0 || new_y >= height)
                    {
                        continue;
                    }

                    error_matrix[new_y][new_x][Color::INDEX_R] += static_cast<int>(current_pixel_error[Color::INDEX_R] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_G] += static_cast<int>(current_pixel_error[Color::INDEX_G] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_B] += static_cast<int>(current_pixel_error[Color::INDEX_B] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                }

                image.set_pixel(palette_nearest, x, y);
            }
        }
    }

    return;
}

// maps the values in the specified threshold matrix to the range 0.0-1.0
std::vector<std::vector<double>> Dither::normalize_threshold_matrix(std::vector<std::vector<int>> threshold_matrix)
{
    size_t height = threshold_matrix.size();
    size_t width = threshold_matrix[0].size();
    std::vector<std::vector<double>> threshold_matrix_normalized = std::vector<std::vector<double>>(height, std::vector<double>(width, 0.0));
    int threshold_matrix_min = INT_MAX;
    int threshold_matrix_max = INT_MIN;
    
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            threshold_matrix_min = std::min(threshold_matrix_min, threshold_matrix[y][x]);
            threshold_matrix_max = std::max(threshold_matrix_max, threshold_matrix[y][x]);
        }
    }

    double threshold_matrix_range = static_cast<double>(threshold_matrix_max) - static_cast<double>(threshold_matrix_min);

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            threshold_matrix_normalized[y][x] = static_cast<double>(threshold_matrix[y][x]) / threshold_matrix_range;
        }
    }

    return threshold_matrix_normalized;
}

// maps the values in the specified threshold matrix to the specified range
std::vector<std::vector<int>> Dither::scale_threshold_matrix(std::vector<std::vector<int>> threshold_matrix, int min, int max)
{
    size_t height = threshold_matrix.size();
    size_t width = threshold_matrix[0].size();
    std::vector<std::vector<int>> threshold_matrix_scaled = std::vector<std::vector<int>>(height, std::vector<int>(width, 0.0));
    int threshold_matrix_min = INT_MAX;
    int threshold_matrix_max = INT_MIN;
    
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            threshold_matrix_min = std::min(threshold_matrix_min, threshold_matrix[y][x]);
            threshold_matrix_max = std::max(threshold_matrix_max, threshold_matrix[y][x]);
        }
    }

    int threshold_matrix_range = threshold_matrix_max - threshold_matrix_min;
    int new_range = max - min;

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            threshold_matrix_scaled[y][x] = (threshold_matrix[y][x] - threshold_matrix_min) * new_range / threshold_matrix_range;
        }
    }

    return threshold_matrix_scaled;
}
