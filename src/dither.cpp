#include "dither.h"
#include "convolve.h"
#include "error.h"
#include "error_diffusion.h"
#include "grayscale.h"
#include "ordered.h"
#include "palette.h"
#include <algorithm> // std::clamp, std::min, std::sort, std::equal_range
#include <cmath> // std::floor, std::lround
#include <random> // std::random_device, std::mt19937, std::uniform_real_distribution, std::discrete_distribution
#include <vector> // std::vector

void Dither::load(const std::string& file_name)
{
    image.load(file_name);
}

void Dither::save(const std::string& file_name) const
{
    image.save(file_name);
}

namespace
{
    // Working buffer: every pixel of frame 0 as float, in sRGB or linear light, plus its
    // alpha. All dither maths happens here; only the chosen palette colour's original sRGB value is
    // written back to the image, so nothing is truncated to 8 bits along the way.
    struct Working
    {
        std::size_t width = 0;
        std::size_t height = 0;
        std::vector<RGBf> pixels;
        std::vector<int> alpha;

        std::size_t index(std::size_t x, std::size_t y) const { return y * width + x; }
        // fully transparent pixels pass through unchanged, spread no error and receive none
        bool transparent(std::size_t i) const { return alpha[i] == 0; }
    };

    Working make_working(const Image& image, const Transfer* transfer)
    {
        Working w;
        w.width = image.get_width();
        w.height = image.get_height();
        w.pixels.resize(w.width * w.height);
        w.alpha.resize(w.width * w.height);

        for(std::size_t y = 0; y < w.height; y++)
        {
            for(std::size_t x = 0; x < w.width; x++)
            {
                const Color color = image.get_pixel(x, y);
                const std::size_t i = w.index(x, y);
                const RGBf value = transfer ? transfer->to_linear(color) : RGBf(color);
                w.pixels[i] = value;
                w.alpha[i] = color.a;
            }
        }

        return w;
    }

    // the palette's original sRGB colour, with the source pixel's alpha kept
    Color output_color(const Palette& palette, std::size_t index_palette, int alpha)
    {
        Color color = palette.get_color_at(index_palette);
        color.a = alpha;
        return color;
    }

    // the blue-noise matrix that gives each pixel its PWM phase
    constexpr const char* PWM_PHASE_MATRIX = "BLUE_NOISE_64X64";

    // Threshold matrix -> offsets centred on 0 in (-0.5, 0.5), using each value's mid-rank in the
    // matrix: (number below + half the number equal) / count - 0.5. For the Bayer and blue-noise
    // matrices (evenly spaced values k * 256 / N) this is exactly (k + 0.5) / N - 0.5. Using ranks
    // rather than raw values also centres the small white-noise matrices, whose random values are
    // unevenly spaced, so every matrix reproduces flat greys at the right coverage.
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

    // UNIFORM_HISTOGRAM: histogram equalisation onto the palette.
    // The palette is sorted darkest to lightest, and every pixel gets a position u in [0, N]
    // along it from the image's luminance CDF: u = N * (pixels darker + half the pixels equal) /
    // pixels. Palette colour k covers [k, k + 1), so reducing with floor(u) uses each colour for
    // about 1/N of the image. Error diffusion and ordered dithering quantise u the same way and
    // carry their error / threshold offset in u (one unit = one palette step), so they keep the
    // same equal shares. Only the luminance order matters, so hue does not pick the colour.
    struct HistogramMap
    {
        // every visible pixel has the same luminance: there is nothing to equalise, and the
        // operations fall back to EUCLIDEAN_DISTANCE
        bool flat = true;
        std::size_t palette_size = 0;
        std::vector<float> position;

        std::size_t rank(float u) const
        {
            const float top = static_cast<float>(palette_size - 1);
            return static_cast<std::size_t>(std::clamp(std::floor(u), 0.0f, top));
        }

        // the centre of rank k, which is what the quantisation error is measured against
        static float centre(std::size_t k) { return static_cast<float>(k) + 0.5f; }
    };

    // BT.709 luminance of a working value (sRGB-encoded, or linear light with -a)
    double luminance(const RGBf& c)
    {
        return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
    }

    HistogramMap make_histogram_map(const Working& w, std::size_t palette_size)
    {
        HistogramMap map;
        map.palette_size = palette_size;
        map.position.assign(w.pixels.size(), 0.0f);
        std::vector<double> sorted;
        sorted.reserve(w.pixels.size());

        for(std::size_t i = 0; i < w.pixels.size(); i++)
        {
            if(!w.transparent(i))
            {
                sorted.push_back(luminance(w.pixels[i]));
            }
        }

        std::sort(sorted.begin(), sorted.end());
        map.flat = sorted.empty() || sorted.front() == sorted.back();

        if(map.flat)
        {
            return map;
        }

        const double count = static_cast<double>(sorted.size());
        const double n = static_cast<double>(palette_size);

        for(std::size_t i = 0; i < w.pixels.size(); i++)
        {
            if(w.transparent(i))
            {
                continue;
            }

            const auto range = std::equal_range(sorted.begin(), sorted.end(), luminance(w.pixels[i]));
            const double below = static_cast<double>(range.first - sorted.begin());
            const double equal = static_cast<double>(range.second - range.first);
            map.position[i] = static_cast<float>(n * (below + equal / 2.0) / count);
        }

        return map;
    }

    struct Tap
    {
        int dx;
        int dy;
        float scalar;
    };

    // Error diffusion over any working value type V (RGBf colours, or float histogram positions).
    // quantise(V) gives a palette index, target(index) the value that index stands for, and the
    // difference is pushed forward onto the not-yet-visited neighbours.
    template <class V, class Quantise, class Target, class Prepare>
    void diffuse(std::vector<V>& values, const Working& w, const std::vector<Tap>& taps, bool serpentine, Prepare prepare, Quantise quantise, Target target, Image& image, const Palette& palette)
    {
        const int image_width = static_cast<int>(w.width);
        const int image_height = static_cast<int>(w.height);

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
                const V value = prepare(values[i]);
                const std::size_t nearest = quantise(value);
                const V error = value - target(nearest);

                for(const Tap& tap : taps)
                {
                    // signed coordinates, so taps to the left of or above the image fail the bounds check
                    const int new_x = x + direction * tap.dx;
                    const int new_y = y + tap.dy;

                    if(new_x < 0 || new_x >= image_width || new_y < 0 || new_y >= image_height)
                    {
                        continue;
                    }

                    const std::size_t j = w.index(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y));

                    if(!w.transparent(j))
                    {
                        values[j] += error * tap.scalar;
                    }
                }

                image.set_pixel(output_color(palette, nearest, w.alpha[i]), static_cast<std::size_t>(x), static_cast<std::size_t>(y));
            }
        }
    }
}

void Dither::grayscale(const std::string& name_grayscale_weights, bool gamma_correction)
{
    const Grayscale grayscale = Grayscale(name_grayscale_weights);
    const Grayscale::Weights& weights = grayscale.weights;
    const Transfer transfer(image.get_gamma());
    const std::size_t height = image.get_height();
    const std::size_t width = image.get_width();

    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            Color color = image.get_pixel(x, y);

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

            image.set_pixel(color, x, y);
        }
    }
}

void Dither::convolve(const std::string& name_kernel, bool gamma_correction)
{
    const Convolve convolve = Convolve(name_kernel);
    // with -a the image is convolved in linear light and encoded back with one rounding
    const Transfer transfer(image.get_gamma());
    const std::vector<std::vector<double>>& kernel = convolve.kernel;
    const int image_height = static_cast<int>(image.get_height());
    const int image_width = static_cast<int>(image.get_width());
    const int kernel_height = static_cast<int>(kernel.size());
    const int kernel_width = static_cast<int>(kernel[0].size());
    const int kernel_height_half = kernel_height / 2;
    const int kernel_width_half = kernel_width / 2;

    // read every source pixel once, so already-convolved pixels are never sampled again
    const std::size_t pixel_count = static_cast<std::size_t>(image_width) * static_cast<std::size_t>(image_height);
    std::vector<RGBf> source(pixel_count);
    std::vector<int> source_alpha(pixel_count);

    for(int y = 0; y < image_height; y++)
    {
        for(int x = 0; x < image_width; x++)
        {
            const Color color = image.get_pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
            const std::size_t i = static_cast<std::size_t>(y * image_width + x);
            source[i] = gamma_correction ? transfer.to_linear(color) : RGBf(color);
            source_alpha[i] = color.a;
        }
    }

    auto to_channel = [](double value) { return static_cast<int>(std::clamp(std::lround(value), 0L, static_cast<long>(Color::CHANNEL_MAX))); };

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
                    const RGBf& sample = source[static_cast<std::size_t>(sy * image_width + sx)];

                    // the kernel is flipped in both directions, which makes this a true convolution rather than a correlation
                    const double weight = kernel[static_cast<std::size_t>(kernel_height - 1 - ky)][static_cast<std::size_t>(kernel_width - 1 - kx)];

                    sum_r += sample.r * weight;
                    sum_g += sample.g * weight;
                    sum_b += sample.b * weight;
                }
            }

            const int alpha = source_alpha[static_cast<std::size_t>(y * image_width + x)];
            const Color color = gamma_correction
                ? transfer.to_encoded(RGBf(static_cast<float>(sum_r), static_cast<float>(sum_g), static_cast<float>(sum_b)), alpha)
                : Color(to_channel(sum_r), to_channel(sum_g), to_channel(sum_b), alpha);
            image.set_pixel(color, static_cast<std::size_t>(x), static_cast<std::size_t>(y));
        }
    }
}

void Dither::reduce(MappingMethod mapping_method, const std::string& name_palette, bool gamma_correction)
{
    const Transfer transfer(image.get_gamma());
    const Transfer* working_space = gamma_correction ? &transfer : nullptr;
    Palette palette = Palette(name_palette);
    palette.set_working_space(working_space);
    const Working w = make_working(image, working_space);
    HistogramMap histogram;

    if(mapping_method == MappingMethod::UNIFORM_HISTOGRAM)
    {
        palette.sort();
        histogram = make_histogram_map(w, palette.size());
    }

    // a flat image has no histogram to equalise, so it maps by distance (nearest_index falls back to Euclidean)
    const bool use_histogram = mapping_method == MappingMethod::UNIFORM_HISTOGRAM && !histogram.flat;

    for(std::size_t y = 0; y < w.height; y++)
    {
        for(std::size_t x = 0; x < w.width; x++)
        {
            const std::size_t i = w.index(x, y);

            if(w.transparent(i))
            {
                continue;
            }

            const std::size_t nearest = use_histogram ? histogram.rank(histogram.position[i]) : palette.nearest_index(w.pixels[i], mapping_method);
            image.set_pixel(output_color(palette, nearest, w.alpha[i]), x, y);
        }
    }
}

void Dither::error_diffusion(const std::string& name_algorithm, ScanPattern scan_pattern, MappingMethod mapping_method, const std::string& name_palette, bool gamma_correction)
{
    const ErrorDiffusion error_diffusion(name_algorithm);
    const Transfer transfer(image.get_gamma());
    const Transfer* working_space = gamma_correction ? &transfer : nullptr;
    Palette palette = Palette(name_palette);
    palette.set_working_space(working_space);
    // the working buffer doubles as the error buffer: diffused error is added straight onto it, in float
    Working w = make_working(image, working_space);
    std::vector<Tap> taps;

    for(const ErrorDiffusion::Tap& tap : error_diffusion.taps)
    {
        taps.push_back({tap.dx, tap.dy, static_cast<float>(tap.scalar)});
    }

    const bool serpentine = scan_pattern == ScanPattern::SERPENTINE;

    if(mapping_method == MappingMethod::UNIFORM_HISTOGRAM)
    {
        palette.sort();
        HistogramMap histogram = make_histogram_map(w, palette.size());

        if(!histogram.flat)
        {
            // diffuse in histogram position; one unit of error is one palette step
            const float top = static_cast<float>(histogram.palette_size);
            diffuse(
                histogram.position, w, taps, serpentine,
                [top](float u) { return std::clamp(u, 0.0f, top); },
                [&histogram](float u) { return histogram.rank(u); },
                [](std::size_t k) { return HistogramMap::centre(k); },
                image, palette);
            return;
        }
    }

    diffuse(
        w.pixels, w, taps, serpentine,
        [](const RGBf& value) { return value.clamped(); },
        [&palette, mapping_method](const RGBf& value) { return palette.nearest_index(value, mapping_method); },
        [&palette](std::size_t k) { return palette.get_working_at(k); },
        image, palette);
}

void Dither::ordered(const std::string& name_threshold_matrix, MappingMethod mapping_method, const std::string& name_palette, bool gamma_correction)
{
    const Ordered ordered = Ordered(name_threshold_matrix);
    const Transfer transfer(image.get_gamma());
    const Transfer* working_space = gamma_correction ? &transfer : nullptr;
    Palette palette = Palette(name_palette);
    palette.set_working_space(working_space);
    palette.sort();

    const Working w = make_working(image, working_space);
    HistogramMap histogram;

    if(mapping_method == MappingMethod::UNIFORM_HISTOGRAM)
    {
        histogram = make_histogram_map(w, palette.size());
    }

    const bool use_histogram = mapping_method == MappingMethod::UNIFORM_HISTOGRAM && !histogram.flat;
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
            std::size_t nearest = 0;

            if(use_histogram)
            {
                // in histogram position a palette step is exactly 1
                nearest = histogram.rank(histogram.position[i] + threshold);
            }
            else
            {
                const RGBf value = w.pixels[i] + palette_pitch_vector * threshold;
                nearest = palette.nearest_index(value, mapping_method);
            }

            image.set_pixel(output_color(palette, nearest, w.alpha[i]), x, y);
        }
    }
}

void Dither::temporal(TemporalMethod temporal_method, std::size_t frames, MappingMethod mapping_method, const std::string& name_palette, bool gamma_correction)
{
    if(frames < 1)
    {
        throw DitherError("temporal dithering needs at least 1 frame");
    }

    // temporal dithering picks colours by per-pixel distance, which UNIFORM_HISTOGRAM is not;
    // CLI rejects it before this point
    if(mapping_method == MappingMethod::UNIFORM_HISTOGRAM)
    {
        throw DitherError("Invalid mapping method for temporal dithering - " + std::string(enum_name(mapping_method)));
    }

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

    const bool manhattan = mapping_method == MappingMethod::MANHATTAN_DISTANCE;
    auto distance = [manhattan](const RGBf& a, const RGBf& b) { return manhattan ? a.distance_manhattan(b) : a.distance_euclidean(b); };

    std::vector<double> weights;

    // deterministic PWM: each pixel starts its run of lighter frames at a phase taken from
    // a blue-noise matrix, so neighbouring pixels with the same duty cycle switch in different
    // frames and every frame shows about the same overall brightness
    std::vector<std::vector<float>> phases;

    if(temporal_method == TemporalMethod::PWM)
    {
        phases = centred_thresholds(Ordered(PWM_PHASE_MATRIX).threshold_matrix);
    }

    for(std::size_t y = 0; y < w.height; y++)
    {
        for(std::size_t x = 0; x < w.width; x++)
        {
            const std::size_t i = w.index(x, y);

            // transparent pixels are copied unchanged into every frame
            if(w.transparent(i))
            {
                const Color original = image.get_pixel(x, y, 0);

                for(std::size_t index_frame = 1; index_frame < image_frames; index_frame++)
                {
                    image.set_pixel(original, x, y, index_frame);
                }

                continue;
            }

            const RGBf& color = w.pixels[i];

            if(temporal_method == TemporalMethod::PWM || temporal_method == TemporalMethod::STOCHASTIC_PWM)
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

                if(temporal_method == TemporalMethod::STOCHASTIC_PWM)
                {
                    // the v1 behaviour: a fresh random draw against the duty cycle in every frame
                    for(std::size_t index_frame = 0; index_frame < image_frames; index_frame++)
                    {
                        const std::size_t chosen = dis(mt) < duty_cycle ? palette_index_lighter : palette_index_darker;
                        image.set_pixel(output_color(palette, chosen, w.alpha[i]), x, y, index_frame);
                    }

                    continue;
                }

                // exactly round(duty * frames) lighter frames, starting at this pixel's phase
                const std::size_t lighter_frames = static_cast<std::size_t>(std::lround(duty_cycle * static_cast<double>(image_frames)));
                const float phase_offset = phases[y % phases.size()][x % phases[0].size()] + 0.5f; // [0, 1)
                const std::size_t phase = std::min(static_cast<std::size_t>(phase_offset * static_cast<float>(image_frames)), image_frames - 1);

                for(std::size_t index_frame = 0; index_frame < image_frames; index_frame++)
                {
                    const bool lighter = (index_frame + image_frames - phase) % image_frames < lighter_frames;
                    image.set_pixel(output_color(palette, lighter ? palette_index_lighter : palette_index_darker, w.alpha[i]), x, y, index_frame);
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
                    image.set_pixel(output_color(palette, dis_disc(mt), w.alpha[i]), x, y, index_frame);
                }
            }
        }
    }
}
