#include "dither.h"

Dither::Dither(std::size_t frames)
{
    image = Image(frames);
    return;
}

std::size_t Dither::load(const char* file_name)
{
    return image.load(file_name);
}

std::size_t Dither::save(const char* file_name)
{
    return image.save(file_name);
}

void Dither::grayscale(std::string name_grayscale_weights, bool gamma_correction)
{
    const Grayscale grayscale = Grayscale(name_grayscale_weights);
    const std::size_t height = image.get_height();
    const std::size_t width = image.get_width();

    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            Color color = image.get_pixel(x, y);

            if(gamma_correction)
            {
                color.to_linear(image.get_gamma());
            }

            color.to_grayscale(grayscale.weights);

            if(gamma_correction)
            {
                color.to_srgb(image.get_gamma());
            }

            image.set_pixel(color, x, y);
        }
    }

    return;
}

void Dither::convolve(std::string name_kernel)
{
    Convolve convolve = Convolve(name_kernel);
    const std::size_t image_height = image.get_height();
    const std::size_t image_width = image.get_width();
    const std::size_t kernel_height = convolve.kernel.size();
    const std::size_t kernel_width = convolve.kernel[0].size();
    const std::size_t kernel_height_half = kernel_height / 2;
    const std::size_t kernel_width_half = kernel_width / 2;
    std::vector<std::vector<int>> image_matrix_r = std::vector<std::vector<int>>(image_height + 2 * kernel_height_half, std::vector<int>(image_width + 2 * kernel_width_half, 0));
    std::vector<std::vector<int>> image_matrix_g = std::vector<std::vector<int>>(image_height + 2 * kernel_height_half, std::vector<int>(image_width + 2 * kernel_width_half, 0));
    std::vector<std::vector<int>> image_matrix_b = std::vector<std::vector<int>>(image_height + 2 * kernel_height_half, std::vector<int>(image_width + 2 * kernel_width_half, 0));

    // pad pixels around the top and bottom of the image
    for(std::size_t x = 0; x < image_width; x++)
    {
        // pad around the top of the image
        for(std::size_t y = -kernel_height_half; y < 0; y++)
        {
            Color color = image.get_pixel(x, 0);
            image_matrix_r[y][x + kernel_width_half] = color.r;
            image_matrix_g[y][x + kernel_width_half] = color.g;
            image_matrix_b[y][x + kernel_width_half] = color.b;
        }

        // pad around the bottom of the image
        for(std::size_t y = image_height; y < image_height + kernel_height_half; y++)
        {
            Color color = image.get_pixel(x, image_height - 1);
            image_matrix_r[y][x + kernel_width_half] = color.r;
            image_matrix_g[y][x + kernel_width_half] = color.g;
            image_matrix_b[y][x + kernel_width_half] = color.b;
        }
    }

    // pad pixels around the left and right borders of the image
    for(std::size_t y = 0; y < image_height; y++)
    {
        // pad around the left border
        for(std::size_t x = -kernel_width_half; x < 0; x++)
        {
            Color color = image.get_pixel(0, y);
            image_matrix_r[y + kernel_height_half][x] = color.r;
            image_matrix_g[y + kernel_height_half][x] = color.g;
            image_matrix_b[y + kernel_height_half][x] = color.b;
        }

        // fill in the center of the image
        for(std::size_t x = 0; x < image_width; x++)
        {
            Color color = image.get_pixel(x, y);
            image_matrix_r[y + kernel_height_half][x + kernel_width_half] = color.r;
            image_matrix_g[y + kernel_height_half][x + kernel_width_half] = color.g;
            image_matrix_b[y + kernel_height_half][x + kernel_width_half] = color.b;
        }

        // pad around the right border
        for(std::size_t x = image_width; x < image_width + kernel_width_half; x++)
        {
            Color color = image.get_pixel(image_width - 1, y);
            image_matrix_r[y + kernel_height_half][x] = color.r;
            image_matrix_g[y + kernel_height_half][x] = color.g;
            image_matrix_b[y + kernel_height_half][x] = color.b;
        }
    }

    std::vector<std::vector<int>> image_matrix_r_convolved = _convolve<int, double>(image_matrix_r, convolve.kernel);
    std::vector<std::vector<int>> image_matrix_g_convolved = _convolve<int, double>(image_matrix_g, convolve.kernel);
    std::vector<std::vector<int>> image_matrix_b_convolved = _convolve<int, double>(image_matrix_b, convolve.kernel);

    for(std::size_t y = 0; y < image_height; y++)
    {
        for(std::size_t x = 0; x < image_width; x++)
        {
            image_matrix_r_convolved[y][x] = std::clamp(image_matrix_r_convolved[y][x], 0, static_cast<int>(Color::CHANNEL_MAX));
            image_matrix_g_convolved[y][x] = std::clamp(image_matrix_g_convolved[y][x], 0, static_cast<int>(Color::CHANNEL_MAX));
            image_matrix_b_convolved[y][x] = std::clamp(image_matrix_b_convolved[y][x], 0, static_cast<int>(Color::CHANNEL_MAX));
            Color color = Color(image_matrix_r_convolved[y][x], image_matrix_g_convolved[y][x], image_matrix_b_convolved[y][x], Color::CHANNEL_MAX);
            image.set_pixel(color, x, y);
        }
    }

    return;
}

void Dither::reduce(std::string name_mapping_method, std::string name_palette, bool gamma_correction)
{
    Palette palette = Palette(name_palette);
    const std::size_t image_height = image.get_height();
    const std::size_t image_width = image.get_width();
    const std::size_t INDEX_COLOR_MIN = 0;
    const std::size_t INDEX_COLOR_MAX = 1;
    std::vector<Color> color_range_image;
    std::vector<Color> color_range_palette;
    std::vector<double> slope;

    if(name_mapping_method == "UNIFORM_HISTOGRAM")
    {
        color_range_image = image.get_color_range(gamma_correction);
        color_range_palette = palette.get_color_range();
        slope = {
            static_cast<double>(color_range_palette[INDEX_COLOR_MAX].r - color_range_palette[INDEX_COLOR_MIN].r) / static_cast<double>(color_range_image[INDEX_COLOR_MAX].r - color_range_image[INDEX_COLOR_MIN].r),
            static_cast<double>(color_range_palette[INDEX_COLOR_MAX].g - color_range_palette[INDEX_COLOR_MIN].g) / static_cast<double>(color_range_image[INDEX_COLOR_MAX].g - color_range_image[INDEX_COLOR_MIN].g),
            static_cast<double>(color_range_palette[INDEX_COLOR_MAX].b - color_range_palette[INDEX_COLOR_MIN].b) / static_cast<double>(color_range_image[INDEX_COLOR_MAX].b - color_range_image[INDEX_COLOR_MIN].b),
        };
    }

    for(std::size_t y = 0; y < image_height; y++)
    {
        for(std::size_t x = 0; x < image_width; x++)
        {
            Color color = image.get_pixel(x, y);

            if(gamma_correction)
            {
                color.to_linear(image.get_gamma());
            }

            Color palette_nearest = palette.nearest(color, name_mapping_method, color_range_image, color_range_palette, slope);

            image.set_pixel(palette_nearest, x, y);
        }
    }

    return;
}

void Dither::error_diffusion(std::string name_algorithm, std::string scan_pattern, std::string name_mapping_method, std::string name_palette, bool gamma_correction)
{
    ErrorDiffusion error_diffusion = ErrorDiffusion(name_algorithm);
    Palette palette = Palette(name_palette);
    const std::size_t image_height = image.get_height();
    const std::size_t image_width = image.get_width();
    std::vector<std::vector<std::vector<int>>> error_matrix(image_height, std::vector<std::vector<int>>(image_width, std::vector<int>(Color::NUM_BYTES - 1, 0)));
    const std::size_t INDEX_COLOR_MIN = 0;
    const std::size_t INDEX_COLOR_MAX = 1;
    std::vector<Color> color_range_image;
    std::vector<Color> color_range_palette;
    std::vector<double> slope;

    if(name_mapping_method == "UNIFORM_HISTOGRAM")
    {
        color_range_image = image.get_color_range(gamma_correction);
        color_range_palette = palette.get_color_range();
        slope = {
            static_cast<double>(color_range_palette[INDEX_COLOR_MAX].r - color_range_palette[INDEX_COLOR_MIN].r) / static_cast<double>(color_range_image[INDEX_COLOR_MAX].r - color_range_image[INDEX_COLOR_MIN].r),
            static_cast<double>(color_range_palette[INDEX_COLOR_MAX].g - color_range_palette[INDEX_COLOR_MIN].g) / static_cast<double>(color_range_image[INDEX_COLOR_MAX].g - color_range_image[INDEX_COLOR_MIN].g),
            static_cast<double>(color_range_palette[INDEX_COLOR_MAX].b - color_range_palette[INDEX_COLOR_MIN].b) / static_cast<double>(color_range_image[INDEX_COLOR_MAX].b - color_range_image[INDEX_COLOR_MIN].b),
        };
    }

    for(std::size_t y = 0; y < image_height; y++)
    {
        // alternate direction on odd rows
        if(scan_pattern == "SERPENTINE" && y % 2 == 1)
        {
            for(int x = image_width - 1; x >= 0; x--)
            {
                // set current pixel to nearest palette color (accounting for accumulated error)
                Color color = image.get_pixel(x, y);

                if(gamma_correction)
                {
                    color.to_linear(image.get_gamma());
                }
                
                color.r += error_matrix[y][x][Color::INDEX_R];
                color.g += error_matrix[y][x][Color::INDEX_G];
                color.b += error_matrix[y][x][Color::INDEX_B];

                color.r = std::clamp(color.r, 0, static_cast<int>(Color::CHANNEL_MAX));
                color.g = std::clamp(color.g, 0, static_cast<int>(Color::CHANNEL_MAX));
                color.b = std::clamp(color.b, 0, static_cast<int>(Color::CHANNEL_MAX));

                Color palette_nearest;

                palette_nearest = palette.nearest(color, name_mapping_method, color_range_image, color_range_palette, slope);

                const std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

                for(std::size_t index_error = 0; index_error < error_diffusion.coordinates.size(); index_error++)
                {
                    const std::size_t new_x = x - error_diffusion.coordinates[index_error].first; // flip x when we are going backwards
                    const std::size_t new_y = y + error_diffusion.coordinates[index_error].second;

                    if(new_x < 0 || new_x >= image_width || new_y < 0 || new_y >= image_height)
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
            for(std::size_t x = 0; x < image_width; x++)
            {
                // set current pixel to nearest palette color (accounting for accumulated error)
                Color color = image.get_pixel(x, y);

                if(gamma_correction)
                {
                    color.to_linear(image.get_gamma());
                }
                
                color.r += error_matrix[y][x][Color::INDEX_R];
                color.g += error_matrix[y][x][Color::INDEX_G];
                color.b += error_matrix[y][x][Color::INDEX_B];

                color.r = std::clamp(color.r, 0, static_cast<int>(Color::CHANNEL_MAX));
                color.g = std::clamp(color.g, 0, static_cast<int>(Color::CHANNEL_MAX));
                color.b = std::clamp(color.b, 0, static_cast<int>(Color::CHANNEL_MAX));

                Color palette_nearest;

                palette_nearest = palette.nearest(color, name_mapping_method, color_range_image, color_range_palette, slope);

                const std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

                for(std::size_t index_error = 0; index_error < error_diffusion.coordinates.size(); index_error++)
                {
                    const std::size_t new_x = x + error_diffusion.coordinates[index_error].first;
                    const std::size_t new_y = y + error_diffusion.coordinates[index_error].second;

                    if(new_x < 0 || new_x >= image_width || new_y < 0 || new_y >= image_height)
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

void Dither::ordered(std::string name_threshold_matrix, std::string name_mapping_method, std::string name_palette, bool gamma_correction)
{
    Ordered ordered = Ordered(name_threshold_matrix);
    Palette palette = Palette(name_palette);
    const std::size_t image_height = image.get_height();
    const std::size_t image_width = image.get_width();
    const std::size_t threshold_matrix_height = ordered.threshold_matrix.size();
    const std::size_t threshold_matrix_width = ordered.threshold_matrix[0].size();

    palette.sort();

    const Color palette_pitch_vector = palette.pitch_vector();
    const int color_channel_max_half = Color::CHANNEL_MAX / 2;
    const std::size_t INDEX_COLOR_MIN = 0;
    const std::size_t INDEX_COLOR_MAX = 1;
    std::vector<Color> color_range_image;
    std::vector<Color> color_range_palette;
    std::vector<double> slope;

    if(name_mapping_method == "UNIFORM_HISTOGRAM")
    {
        color_range_image = image.get_color_range(gamma_correction);
        color_range_palette = palette.get_color_range();
        slope = {
            static_cast<double>(color_range_palette[INDEX_COLOR_MAX].r - color_range_palette[INDEX_COLOR_MIN].r) / static_cast<double>(color_range_image[INDEX_COLOR_MAX].r - color_range_image[INDEX_COLOR_MIN].r),
            static_cast<double>(color_range_palette[INDEX_COLOR_MAX].g - color_range_palette[INDEX_COLOR_MIN].g) / static_cast<double>(color_range_image[INDEX_COLOR_MAX].g - color_range_image[INDEX_COLOR_MIN].g),
            static_cast<double>(color_range_palette[INDEX_COLOR_MAX].b - color_range_palette[INDEX_COLOR_MIN].b) / static_cast<double>(color_range_image[INDEX_COLOR_MAX].b - color_range_image[INDEX_COLOR_MIN].b),
        };
    }

    for(std::size_t y = 0; y < image_height; y++)
    {
        for(std::size_t x = 0; x < image_width; x++)
        {
            Color color = image.get_pixel(x, y);

            if(gamma_correction)
            {
                color.to_linear(image.get_gamma());
            }
            
            const int threshold_value = ordered.threshold_matrix[y % threshold_matrix_height][x % threshold_matrix_width];
            // normalize the threshold value by subtracting half the color channel range (normalized values should range from -127 to 128)
            const int threshold_value_scaled = threshold_value - color_channel_max_half;
            // create a color offset at each position that is the average spread in the color space (palette_pitch_vector / color_channel_max) multiplied by the normalized threshold
            const Color threshold_color_offset = Color(
                threshold_value_scaled * palette_pitch_vector.r / Color::CHANNEL_MAX,
                threshold_value_scaled * palette_pitch_vector.g / Color::CHANNEL_MAX,
                threshold_value_scaled * palette_pitch_vector.b / Color::CHANNEL_MAX,
                Color::CHANNEL_MAX);

            const Color palette_nearest = palette.nearest(color.add(threshold_color_offset), name_mapping_method, color_range_image, color_range_palette, slope);
            
            image.set_pixel(palette_nearest, x, y);
        }
    }

    return;
}

void Dither::temporal(std::string name_temporal_method, std::size_t frames, std::string name_mapping_method, std::string name_palette, bool gamma_correction)
{
    Palette palette = Palette(name_palette);
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    palette.sort();
    image.set_frames(frames);
    
    const std::size_t image_height = image.get_height();
    const std::size_t image_width = image.get_width();
    const std::size_t image_frames = image.get_frames();

    if(name_temporal_method == "PWM")
    {
        for(std::size_t y = 0; y < image_height; y++)
        {
            for(std::size_t x = 0; x < image_width; x++)
            {
                // calculate the pixel's similarity to the two nearest colors in the palette
                Color color = image.get_pixel(x, y);

                if(gamma_correction)
                {
                    color.to_linear(image.get_gamma());
                }

                const int index_darker = palette.nearest_index_darker(color);
                const int index_lighter = palette.nearest_index_lighter(color);
                Color palette_color_darker;
                Color palette_color_lighter;
                double duty_cycle = 0.0; // the proportion of the time that the pixel is set to nearest lighter palette color

                // no darker colors on the palette, so set duty cycle to 0%
                if(index_darker < 0)
                {
                    palette_color_darker = palette.get_color_at(0);
                    duty_cycle = 0.0;
                }
                // no lighter colors on th epalette, so set duty cycle to 100%
                else if(index_lighter < 0)
                {
                    palette_color_lighter = palette.get_color_at(palette.size() - 1);
                    duty_cycle = 1.0;
                }
                else
                {
                    palette_color_darker = palette.get_color_at(index_darker);
                    palette_color_lighter = palette.get_color_at(index_lighter);
                    double distance_darker;
                    double distance_lighter;

                    if(name_mapping_method == "EUCLIDEAN_DISTANCE")
                    {
                        distance_darker = color.distance_euclidean(palette_color_darker);
                        distance_lighter = color.distance_euclidean(palette_color_lighter);
                    }
                    else if(name_mapping_method == "MANHATTAN_DISTANCE")
                    {
                        distance_darker = color.distance_manhattan(palette_color_darker);
                        distance_lighter = color.distance_manhattan(palette_color_lighter);
                    }
                    
                    // larger dark distance -> larger light time duty cycle
                    duty_cycle = distance_darker / (distance_darker + distance_lighter);
                }

                for(std::size_t index_frame = 0; index_frame < image_frames; index_frame++)
                {
                    if(dis(mt) < duty_cycle)
                    {
                        image.set_pixel(palette_color_lighter, x, y, index_frame);
                    }
                    else
                    {
                        image.set_pixel(palette_color_darker, x, y, index_frame);
                    }
                }
            }
        }
    }
    else if(name_temporal_method == "RANDOM")
    {
        std::vector<double> weights;
        std::size_t palette_size = palette.size();

        for(std::size_t y = 0; y < image_height; y++)
        {
            for(std::size_t x = 0; x < image_width; x++)
            {
                // calculate the pixel's similarity to each color in the palette
                Color color = image.get_pixel(x, y);
                weights.resize(0);

                if(gamma_correction)
                {
                    color.to_linear(image.get_gamma());
                }

                for(std::size_t index_palette = 0; index_palette < palette_size; index_palette++)
                {
                    if(name_mapping_method == "EUCLIDEAN_DISTANCE")
                    {
                        weights.push_back(1.0 / color.distance_euclidean(palette.get_color_at(index_palette)));
                    }
                    else if(name_mapping_method == "MANHATTAN_DISTANCE")
                    {
                        weights.push_back(1.0 / color.distance_manhattan(palette.get_color_at(index_palette)));
                    }
                }

                std::discrete_distribution<> dis_disc(weights.begin(), weights.end());

                for(std::size_t index_frame = 0; index_frame < image_frames; index_frame++)
                {
                    image.set_pixel(palette.get_color_at(dis_disc(mt)), x, y, index_frame);
                }
                
            }
        }
    }
    else
    {
        std::cerr << "Error: Invalid temporal dithering method: " << name_temporal_method << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return;
}
