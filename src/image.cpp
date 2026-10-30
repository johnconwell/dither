#include "image.h"
#include "error.h"
#include "gif.h"
#include "lodepng.h"
#include <algorithm> // std::transform
#include <cctype> // std::tolower
#include <cstdint> // uint32_t
#include <filesystem> // std::filesystem::path

Color Image::get_pixel(std::size_t x, std::size_t y, std::size_t frame) const
{
    const std::size_t index_start = Color::NUM_BYTES * (width * y + x);
    const std::vector<unsigned char>& data = pixels[frame];
    return Color(data[index_start + Color::INDEX_R], data[index_start + Color::INDEX_G], data[index_start + Color::INDEX_B], data[index_start + Color::INDEX_A]);
}

void Image::set_pixel(const Color& color, std::size_t x, std::size_t y, std::size_t frame)
{
    const std::size_t index_start = Color::NUM_BYTES * (width * y + x);
    std::vector<unsigned char>& data = pixels[frame];
    data[index_start + Color::INDEX_R] = static_cast<unsigned char>(color.r);
    data[index_start + Color::INDEX_G] = static_cast<unsigned char>(color.g);
    data[index_start + Color::INDEX_B] = static_cast<unsigned char>(color.b);
    data[index_start + Color::INDEX_A] = static_cast<unsigned char>(color.a);
}

std::size_t Image::get_width() const
{
    return width;
}

std::size_t Image::get_height() const
{
    return height;
}

double Image::get_gamma() const
{
    return gamma;
}

std::size_t Image::get_frames() const
{
    return pixels.size();
}

void Image::set_frames(std::size_t new_frames)
{
    pixels.resize(std::max<std::size_t>(new_frames, 1), std::vector<unsigned char>(static_cast<std::size_t>(width) * height * Color::NUM_BYTES));
}

void Image::load(const std::string& file_name)
{
    std::vector<unsigned char> png_buffer;
    std::vector<unsigned char> decoded;
    lodepng::State state;
    unsigned int decoded_width = 0;
    unsigned int decoded_height = 0;

    unsigned int error = lodepng::load_file(png_buffer, file_name);

    if(!error)
    {
        error = lodepng::decode(decoded, decoded_width, decoded_height, state, png_buffer);
    }

    if(error)
    {
        throw DitherError("load " + file_name + " - " + std::to_string(error) + ": " + lodepng_error_text(error));
    }

    pixels.assign(1, std::move(decoded));
    width = decoded_width;
    height = decoded_height;

    // An sRGB chunk, or no colour information at all, means the standard sRGB curve (gamma = 0).
    // Only a gAMA chunk without an sRGB chunk selects a pure power curve with the file's exponent.
    if(state.info_png.gama_defined && !state.info_png.srgb_defined && state.info_png.gama_gamma > 0)
    {
        gamma = 100000.0 / static_cast<double>(state.info_png.gama_gamma);
    }
    else
    {
        gamma = 0.0;
    }
}

void Image::save(const std::string& file_name) const
{
    // decide the format from the extension (case-insensitive)
    std::string extension = std::filesystem::path(file_name).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if(pixels.empty() || pixels[0].empty())
    {
        throw DitherError("no image frames to save - " + file_name);
    }

    if(extension == ".png")
    {
        unsigned int error = 0;

        // save as a single frame png
        if(pixels.size() == 1)
        {
            error = lodepng::encode(file_name, pixels[0], width, height);
        }
        // save as a multi-frame png (sprite sheet)
        else
        {
            std::vector<unsigned char> combined;

            // combine each frame into a contiguous array
            for(const std::vector<unsigned char>& frame : pixels)
            {
                combined.insert(combined.end(), frame.begin(), frame.end());
            }

            error = lodepng::encode(file_name, combined, width, height * static_cast<unsigned int>(pixels.size()));
        }

        if(error)
        {
            throw DitherError("png save " + file_name + " - " + std::to_string(error) + ": " + lodepng_error_text(error));
        }
    }
    else if(extension == ".gif")
    {
        const uint32_t DELAY = 2; // many gif viewers will ignore delay values < 2
        const int32_t BIT_DEPTH = 8;
        const bool DITHER = false;
        GifWriter writer = {};

        if(!GifBegin(&writer, file_name.c_str(), width, height, DELAY, BIT_DEPTH, DITHER))
        {
            throw DitherError("gif save - could not open " + file_name);
        }

        bool gif_save_success = true;

        for(const std::vector<unsigned char>& frame : pixels)
        {
            gif_save_success = GifWriteFrame(&writer, frame.data(), width, height, DELAY, BIT_DEPTH, DITHER) && gif_save_success;
        }

        const bool gif_end_success = GifEnd(&writer);

        if(!gif_save_success)
        {
            throw DitherError("gif save - " + file_name);
        }

        if(!gif_end_success)
        {
            throw DitherError("gif end - " + file_name);
        }
    }
    else
    {
        throw DitherError("unsupported output file format (expected .png or .gif) - " + file_name);
    }
}
