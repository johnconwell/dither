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

namespace
{
    // Phase 3 working buffer: every pixel of frame 0 as float, in sRGB or linear light, plus its
    // alpha. All dither maths happens here; only the chosen palette colour's original sRGB value is
    // written back to the image, so nothing is truncated to 8 bits along the way (B3, Q1, Q6).
    struct Working
    {
        std::size_t width = 0;
        std::size_t height = 0;
        std::vector<RGBf> pixels;
        std::vector<int> alpha;
        // range over the non-transparent pixels, for UNIFORM_HISTOGRAM
        RGBf min;
        RGBf max;

        std::size_t index(std::size_t x, std::size_t y) const { return y * width + x; }
        // fully transparent pixels pass through unchanged, spread no error and receive none (D3)
        bool transparent(std::size_t i) const { return alpha[i] == 0; }
    };

    Working make_working(Image& image, const Transfer* transfer)
    {
        Working w;
        w.width = image.get_width();
        w.height = image.get_height();
        w.pixels.resize(w.width * w.height);
        w.alpha.resize(w.width * w.height);

        constexpr float INF = std::numeric_limits<float>::infinity();
        w.min = RGBf(INF, INF, INF);
        w.max = RGBf(-INF, -INF, -INF);

        for(std::size_t y = 0; y < w.height; y++)
        {
            for(std::size_t x = 0; x < w.width; x++)
            {
                const Color color = image.get_pixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y));
                const std::size_t i = w.index(x, y);
                const RGBf value = transfer ? transfer->to_linear(color) : RGBf(color);
                w.pixels[i] = value;
                w.alpha[i] = color.a;

                if(color.a != 0)
                {
                    w.min = RGBf(std::min(w.min.r, value.r), std::min(w.min.g, value.g), std::min(w.min.b, value.b));
                    w.max = RGBf(std::max(w.max.r, value.r), std::max(w.max.g, value.g), std::max(w.max.b, value.b));
                }
            }
        }

        return w;
    }

    // the palette's original sRGB colour, with the source pixel's alpha kept (D3)
    Color output_color(const Palette& palette, std::size_t index_palette, int alpha)
    {
        Color color = palette.get_color_at(index_palette);
        color.a = alpha;
        return color;
    }

    // Threshold matrix -> offsets centred on 0 in (-0.5, 0.5), using each value's mid-rank in the
    // matrix: (number below + half the number equal) / count - 0.5. For the Bayer and blue-noise
    // matrices (evenly spaced values k * 256 / N) this is exactly (k + 0.5) / N - 0.5, which removes
    // the old "t - 127" bias (Q3); for the small white-noise matrices it also removes the bias of
    // their uneven random values, so every matrix reproduces flat greys at the right coverage.
    std::vector<std::vector<float>> centred_thresholds(const std::vector<std::vector<double>>& matrix)
    {
        std::vector<double> sorted;

        for(const auto& row : matrix)
        {
            sorted.insert(sorted.end(), row.begin(), row.end());
        }

        std::sort(sorted.begin(), sorted.end());
        const double count = static_cast<double>(sorted.size());
        std::vector<std::vector<float>> offsets(matrix.size());

        for(std::size_t y = 0; y < matrix.size(); y++)
        {
            for(const double t : matrix[y])
            {
                const auto range = std::equal_range(sorted.begin(), sorted.end(), t);
                const double below = static_cast<double>(range.first - sorted.begin());
                const double equal = static_cast<double>(range.second - range.first);
                offsets[y].push_back(static_cast<float>((below + equal / 2.0) / count - 0.5));
            }
        }

        return offsets;
    }
}

void Dither::grayscale(std::string name_grayscale_weights, bool gamma_correction)
{
    const Grayscale grayscale = Grayscale(name_grayscale_weights);
    const std::vector<double>& weights = grayscale.weights;
    const Transfer transfer(image.get_gamma());
    const std::size_t height = image.get_height();
    const std::size_t width = image.get_width();

    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            Color color = image.get_pixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y));

            if(gamma_correction)
            {
                // weight the channels in linear light and round once, back in sRGB
                const RGBf linear = transfer.to_linear(color);
                const double value = linear.r * weights[Color::INDEX_R] + linear.g * weights[Color::INDEX_G] + linear.b * weights[Color::INDEX_B];
                const int encoded = transfer.to_encoded(static_cast<float>(value));
                color = Color(encoded, encoded, encoded, color.a);
            }
            else
            {
                color.to_grayscale(weights);
            }

            image.set_pixel(color, static_cast<unsigned int>(x), static_cast<unsigned int>(y));
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
    const Transfer transfer(image.get_gamma());
    const Transfer* working_space = gamma_correction ? &transfer : nullptr;
    Palette palette = Palette(name_palette);
    palette.set_working_space(working_space);
    const Working w = make_working(image, working_space);
    const Palette::RangeMap range_map = palette.range_map(w.min, w.max);

    for(std::size_t y = 0; y < w.height; y++)
    {
        for(std::size_t x = 0; x < w.width; x++)
        {
            const std::size_t i = w.index(x, y);

            if(w.transparent(i))
            {
                continue;
            }

            const std::size_t nearest = palette.nearest_index(w.pixels[i], name_mapping_method, &range_map);
            image.set_pixel(output_color(palette, nearest, w.alpha[i]), static_cast<unsigned int>(x), static_cast<unsigned int>(y));
        }
    }

    return;
}

void Dither::error_diffusion(std::string name_algorithm, std::string scan_pattern, std::string name_mapping_method, std::string name_palette, bool gamma_correction)
{
    ErrorDiffusion error_diffusion = ErrorDiffusion(name_algorithm);
    const Transfer transfer(image.get_gamma());
    const Transfer* working_space = gamma_correction ? &transfer : nullptr;
    Palette palette = Palette(name_palette);
    palette.set_working_space(working_space);
    // the working buffer doubles as the error buffer: diffused error is added straight onto it, in float (Q1)
    Working w = make_working(image, working_space);
    const Palette::RangeMap range_map = palette.range_map(w.min, w.max);

    struct Tap
    {
        int dx;
        int dy;
        float scalar;
    };
    std::vector<Tap> taps;

    for(const auto& coordinate : error_diffusion.coordinates)
    {
        taps.push_back({coordinate.first, coordinate.second, static_cast<float>(error_diffusion.scalars.at(coordinate))});
    }

    const int image_width = static_cast<int>(w.width);
    const int image_height = static_cast<int>(w.height);
    const bool serpentine = scan_pattern == "SERPENTINE";

    for(int y = 0; y < image_height; y++)
    {
        // serpentine: odd rows run right to left, and the kernel is mirrored with them
        const int direction = (serpentine && y % 2 == 1) ? -1 : 1;
        const int x_start = direction > 0 ? 0 : image_width - 1;

        for(int step = 0; step < image_width; step++)
        {
            const int x = x_start + direction * step;
            const std::size_t i = w.index(static_cast<std::size_t>(x), static_cast<std::size_t>(y));

            if(w.transparent(i))
            {
                continue;
            }

            // set current pixel to nearest palette color (accounting for accumulated error)
            const RGBf value = w.pixels[i].clamped();
            const std::size_t nearest = palette.nearest_index(value, name_mapping_method, &range_map);
            const RGBf error = value - palette.get_working_at(nearest);

            for(const Tap& tap : taps)
            {
                // signed coordinates, so the bounds check is real (Q12)
                const int new_x = x + direction * tap.dx;
                const int new_y = y + tap.dy;

                if(new_x < 0 || new_x >= image_width || new_y < 0 || new_y >= image_height)
                {
                    continue;
                }

                const std::size_t j = w.index(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y));

                if(!w.transparent(j))
                {
                    w.pixels[j] += error * tap.scalar;
                }
            }

            image.set_pixel(output_color(palette, nearest, w.alpha[i]), static_cast<unsigned int>(x), static_cast<unsigned int>(y));
        }
    }

    return;
}

void Dither::ordered(std::string name_threshold_matrix, std::string name_mapping_method, std::string name_palette, bool gamma_correction)
{
    const Ordered ordered = Ordered(name_threshold_matrix);
    const Transfer transfer(image.get_gamma());
    const Transfer* working_space = gamma_correction ? &transfer : nullptr;
    Palette palette = Palette(name_palette);
    palette.set_working_space(working_space);
    palette.sort();

    const Working w = make_working(image, working_space);
    const Palette::RangeMap range_map = palette.range_map(w.min, w.max);
    const std::vector<std::vector<float>> thresholds = centred_thresholds(ordered.threshold_matrix);
    const std::size_t threshold_matrix_height = thresholds.size();
    const std::size_t threshold_matrix_width = thresholds[0].size();
    // the average step between neighbouring palette colours, in working space
    const RGBf palette_pitch_vector = palette.pitch_vector();

    for(std::size_t y = 0; y < w.height; y++)
    {
        for(std::size_t x = 0; x < w.width; x++)
        {
            const std::size_t i = w.index(x, y);

            if(w.transparent(i))
            {
                continue;
            }

            // offset each pixel by up to half a palette step either way, following the centred threshold
            const float threshold = thresholds[y % threshold_matrix_height][x % threshold_matrix_width];
            const RGBf value = w.pixels[i] + palette_pitch_vector * threshold;
            const std::size_t nearest = palette.nearest_index(value, name_mapping_method, &range_map);

            image.set_pixel(output_color(palette, nearest, w.alpha[i]), static_cast<unsigned int>(x), static_cast<unsigned int>(y));
        }
    }

    return;
}

void Dither::temporal(std::string name_temporal_method, std::size_t frames, std::string name_mapping_method, std::string name_palette, bool gamma_correction)
{
    const Transfer transfer(image.get_gamma());
    const Transfer* working_space = gamma_correction ? &transfer : nullptr;
    Palette palette = Palette(name_palette);
    palette.set_working_space(working_space);
    palette.sort();

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    const Working w = make_working(image, working_space);
    image.set_frames(frames);
    const std::size_t image_frames = image.get_frames();
    const std::size_t palette_size = palette.size();

    auto distance = [&](const RGBf& a, const RGBf& b) {
        if(name_mapping_method == "EUCLIDEAN_DISTANCE")
        {
            return a.distance_euclidean(b);
        }

        if(name_mapping_method == "MANHATTAN_DISTANCE")
        {
            return a.distance_manhattan(b);
        }

        std::cerr << "Error: Invalid mapping method for temporal dithering - " << name_mapping_method << std::endl;
        std::exit(EXIT_FAILURE);
    };

    if(name_temporal_method != "PWM" && name_temporal_method != "RANDOM")
    {
        std::cerr << "Error: Invalid temporal dithering method: " << name_temporal_method << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::vector<double> weights;

    for(std::size_t y = 0; y < w.height; y++)
    {
        for(std::size_t x = 0; x < w.width; x++)
        {
            const std::size_t i = w.index(x, y);
            const unsigned int ux = static_cast<unsigned int>(x);
            const unsigned int uy = static_cast<unsigned int>(y);

            // transparent pixels are copied unchanged into every frame
            if(w.transparent(i))
            {
                const Color original = image.get_pixel(ux, uy, 0);

                for(std::size_t index_frame = 1; index_frame < image_frames; index_frame++)
                {
                    image.set_pixel(original, ux, uy, index_frame);
                }

                continue;
            }

            const RGBf& color = w.pixels[i];

            if(name_temporal_method == "PWM")
            {
                // modulate between the nearest darker and lighter palette colours
                const int index_darker = palette.nearest_index_darker(color);
                const int index_lighter = palette.nearest_index_lighter(color);
                std::size_t palette_index_darker = 0;
                std::size_t palette_index_lighter = palette_size - 1;
                double duty_cycle = 0.0; // the proportion of the time that the pixel is set to nearest lighter palette color

                // no darker colors on the palette, so set duty cycle to 0%
                if(index_darker < 0)
                {
                    duty_cycle = 0.0;
                }
                // no lighter colors on the palette, so set duty cycle to 100%
                else if(index_lighter < 0)
                {
                    duty_cycle = 1.0;
                }
                else
                {
                    palette_index_darker = static_cast<std::size_t>(index_darker);
                    palette_index_lighter = static_cast<std::size_t>(index_lighter);
                    const double distance_darker = distance(color, palette.get_working_at(palette_index_darker));
                    const double distance_lighter = distance(color, palette.get_working_at(palette_index_lighter));

                    // larger dark distance -> larger light time duty cycle
                    const double distance_total = distance_darker + distance_lighter;
                    duty_cycle = distance_total > 0.0 ? distance_darker / distance_total : 0.0;
                }

                for(std::size_t index_frame = 0; index_frame < image_frames; index_frame++)
                {
                    const std::size_t chosen = dis(mt) < duty_cycle ? palette_index_lighter : palette_index_darker;
                    image.set_pixel(output_color(palette, chosen, w.alpha[i]), ux, uy, index_frame);
                }
            }
            else
            {
                // RANDOM: pick a palette colour each frame with probability proportional to 1 / distance
                weights.assign(palette_size, 0.0);
                bool exact_match = false;

                for(std::size_t index_palette = 0; index_palette < palette_size; index_palette++)
                {
                    const double d = distance(color, palette.get_working_at(index_palette));

                    // a pixel that equals a palette colour would get an infinite weight; give that colour 100% instead
                    if(d <= 0.0)
                    {
                        if(!exact_match)
                        {
                            std::fill(weights.begin(), weights.end(), 0.0);
                            exact_match = true;
                        }

                        weights[index_palette] = 1.0;
                    }
                    else if(!exact_match)
                    {
                        weights[index_palette] = 1.0 / d;
                    }
                }

                std::discrete_distribution<std::size_t> dis_disc(weights.begin(), weights.end());

                for(std::size_t index_frame = 0; index_frame < image_frames; index_frame++)
                {
                    image.set_pixel(output_color(palette, dis_disc(mt), w.alpha[i]), ux, uy, index_frame);
                }
            }
        }
    }

    return;
}
