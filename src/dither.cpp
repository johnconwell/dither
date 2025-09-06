#include "dither.h"

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
    size_t height = image.get_height();
    size_t width = image.get_width();

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            Color color = image.get_pixel(x, y);
            Color threshold = Color(static_cast<int16_t>(threshold_matrix[y][x]), static_cast<int16_t>(threshold_matrix[y][x]), static_cast<int16_t>(threshold_matrix[y][x]), Color::CHANNEL_MAX);
            
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
