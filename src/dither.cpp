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
    const Convolve convolve = Convolve(name_kernel);
    const std::vector<std::vector<double>>& kernel = convolve.kernel;
    const int image_height = static_cast<int>(image.get_height());
    const int image_width = static_cast<int>(image.get_width());
    const int kernel_height = static_cast<int>(kernel.size());
    const int kernel_width = static_cast<int>(kernel[0].size());
    const int kernel_height_half = kernel_height / 2;
    const int kernel_width_half = kernel_width / 2;
    constexpr int CHANNEL_MAX = static_cast<int>(Color::CHANNEL_MAX);

    // read every source pixel once, so already-convolved pixels are never sampled again
    std::vector<Color> source(static_cast<std::size_t>(image_width) * static_cast<std::size_t>(image_height));

    for(int y = 0; y < image_height; y++)
    {
        for(int x = 0; x < image_width; x++)
        {
            source[static_cast<std::size_t>(y * image_width + x)] = image.get_pixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y));
        }
    }

    auto to_channel = [](double value) { return static_cast<int>(std::clamp(std::lround(value), 0L, static_cast<long>(CHANNEL_MAX))); };

    for(int y = 0; y < image_height; y++)
    {
        for(int x = 0; x < image_width; x++)
        {
            double sum_r = 0.0;
            double sum_g = 0.0;
            double sum_b = 0.0;

            for(int ky = 0; ky < kernel_height; ky++)
            {
                // samples outside the image take the value of the nearest edge pixel (clamp-to-edge)
                const int sy = std::clamp(y + ky - kernel_height_half, 0, image_height - 1);

                for(int kx = 0; kx < kernel_width; kx++)
                {
                    const int sx = std::clamp(x + kx - kernel_width_half, 0, image_width - 1);
                    const Color& sample = source[static_cast<std::size_t>(sy * image_width + sx)];

                    // the kernel is flipped in both directions, which makes this a true convolution rather than a correlation
                    const double weight = kernel[static_cast<std::size_t>(kernel_height - 1 - ky)][static_cast<std::size_t>(kernel_width - 1 - kx)];

                    sum_r += sample.r * weight;
                    sum_g += sample.g * weight;
                    sum_b += sample.b * weight;
                }
            }

            const Color& original = source[static_cast<std::size_t>(y * image_width + x)];
            const Color color = Color(to_channel(sum_r), to_channel(sum_g), to_channel(sum_b), original.a);
            image.set_pixel(color, static_cast<unsigned int>(x), static_cast<unsigned int>(y));
        }
    }

    return;
}

void Dither::reduce(std::string name_mapping_method, std::string name_palette, bool gamma_correction)
{
    Palette palette = Palette(name_palette);
    const std::size_t image_height = image.get_height();
    const std::size_t image_width = image.get_width();
    std::vector<Color> color_range_image;
    std::vector<Color> color_range_palette;
    std::vector<double> slope;

    if(name_mapping_method == "UNIFORM_HISTOGRAM")
    {
        color_range_image = image.get_color_range(gamma_correction);
        color_range_palette = palette.get_color_range();
        slope = Palette::range_slope(color_range_image, color_range_palette);
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
    std::vector<Color> color_range_image;
    std::vector<Color> color_range_palette;
    std::vector<double> slope;

    if(name_mapping_method == "UNIFORM_HISTOGRAM")
    {
        color_range_image = image.get_color_range(gamma_correction);
        color_range_palette = palette.get_color_range();
        slope = Palette::range_slope(color_range_image, color_range_palette);
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
    std::vector<Color> color_range_image;
    std::vector<Color> color_range_palette;
    std::vector<double> slope;

    if(name_mapping_method == "UNIFORM_HISTOGRAM")
    {
        color_range_image = image.get_color_range(gamma_correction);
        color_range_palette = palette.get_color_range();
        slope = Palette::range_slope(color_range_image, color_range_palette);
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
            // keep this arithmetic signed: Color::CHANNEL_MAX is a size_t, and mixing it with a negative
            // threshold_value_scaled would convert the product to unsigned and wrap around (B1)
            constexpr int CHANNEL_MAX = static_cast<int>(Color::CHANNEL_MAX);
            const Color threshold_color_offset = Color(
                threshold_value_scaled * palette_pitch_vector.r / CHANNEL_MAX,
                threshold_value_scaled * palette_pitch_vector.g / CHANNEL_MAX,
                threshold_value_scaled * palette_pitch_vector.b / CHANNEL_MAX,
                CHANNEL_MAX);

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
                    double distance_darker = 0.0;
                    double distance_lighter = 0.0;

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
                    else
                    {
                        std::cerr << "Error: Invalid mapping method for temporal dithering - " << name_mapping_method << std::endl;
                        std::exit(EXIT_FAILURE);
                    }
                    
                    // larger dark distance -> larger light time duty cycle
                    const double distance_total = distance_darker + distance_lighter;
                    duty_cycle = distance_total > 0.0 ? distance_darker / distance_total : 0.0;
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

                bool exact_match = false;

                for(std::size_t index_palette = 0; index_palette < palette_size; index_palette++)
                {
                    double distance;

                    if(name_mapping_method == "EUCLIDEAN_DISTANCE")
                    {
                        distance = color.distance_euclidean(palette.get_color_at(index_palette));
                    }
                    else if(name_mapping_method == "MANHATTAN_DISTANCE")
                    {
                        distance = color.distance_manhattan(palette.get_color_at(index_palette));
                    }
                    else
                    {
                        std::cerr << "Error: Invalid mapping method for temporal dithering - " << name_mapping_method << std::endl;
                        std::exit(EXIT_FAILURE);
                    }

                    // weights are 1 / distance, so a pixel that equals a palette colour would get an infinite weight;
                    // give that colour 100% probability instead
                    if(distance <= 0.0)
                    {
                        if(!exact_match)
                        {
                            std::fill(weights.begin(), weights.end(), 0.0);
                            exact_match = true;
                        }

                        weights.push_back(1.0);
                    }
                    else
                    {
                        weights.push_back(exact_match ? 0.0 : 1.0 / distance);
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
