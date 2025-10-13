#include "image.h"

Image::Image()
{
    pixels.resize(0);
    width = 0;
    height = 0;
    gamma = 0.0;
    frames = 1;
    return;
}

Image::Image(std::size_t frames)
{
    this->pixels.resize(frames);
    
    for(std::size_t index_frame = 0; index_frame < frames; index_frame++)
    {
        this->pixels[index_frame].resize(0);
    }

    this->width = 0;
    this->height = 0;
    this->gamma = 0.0;
    this->frames = frames;

    return;
}

void Image::reset()
{
    pixels.resize(frames);

    for(std::size_t index_frame = 0; index_frame < frames; index_frame++)
    {
        this->pixels[index_frame].resize(0);
    }

    width = 0;
    height = 0;
    gamma = 0.0;
    
    return;
}

Color Image::get_pixel(unsigned int x, unsigned int y, std::size_t frame)
{
    const int index_start = Color::NUM_BYTES * width * y + Color::NUM_BYTES * x;
    return Color(
        pixels[frame][index_start + Color::INDEX_R],
        pixels[frame][index_start + Color::INDEX_G],
        pixels[frame][index_start + Color::INDEX_B],
        pixels[frame][index_start + Color::INDEX_A]
    );
}

void Image::set_pixel(Color color, unsigned int x, unsigned int y, std::size_t frame)
{
    const int index_start = Color::NUM_BYTES * width * y + Color::NUM_BYTES * x;
    pixels[frame][index_start + Color::INDEX_R] = color.r;
    pixels[frame][index_start + Color::INDEX_G] = color.g;
    pixels[frame][index_start + Color::INDEX_B] = color.b;
    pixels[frame][index_start + Color::INDEX_A] = color.a;
    return;
}

std::size_t Image::get_width()
{
    return width;
}

std::size_t Image::get_height()
{
    return height;
}

double Image::get_gamma()
{
    return gamma;
}

std::size_t Image::get_frames()
{
    return frames;
}

void Image::set_frames(std::size_t new_frames)
{
    const std::size_t current_frames = frames;
    frames = new_frames;

    pixels.resize(frames);

    if(new_frames > current_frames)
    {
        for(std::size_t index_frame = current_frames; index_frame < frames; index_frame++)
        {
            pixels[index_frame].resize(width * height * Color::NUM_BYTES);
        }
    }

    return;
}

std::vector<Color> Image::get_color_range(bool gamma_correction)
{
    std::vector<Color> color_range = {
        Color(Color::CHANNEL_MAX, Color::CHANNEL_MAX, Color::CHANNEL_MAX, Color::CHANNEL_MAX),
        Color(0, 0, 0, Color::CHANNEL_MAX)
    };
    const std::size_t INDEX_COLOR_MIN = 0;
    const std::size_t INDEX_COLOR_MAX = 1;

    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            Color color = get_pixel(x, y);

            if(gamma_correction)
            {
                color.to_linear(gamma);
            }

            if(color.r < color_range[INDEX_COLOR_MIN].r)
            {
                color_range[INDEX_COLOR_MIN].r = color.r;
            }
            else if(color.r > color_range[INDEX_COLOR_MAX].r)
            {
                color_range[INDEX_COLOR_MAX].r = color.r;
            }

            if(color.g < color_range[INDEX_COLOR_MIN].g)
            {
                color_range[INDEX_COLOR_MIN].g = color.g;
            }
            else if(color.g > color_range[INDEX_COLOR_MAX].g)
            {
                color_range[INDEX_COLOR_MAX].g = color.g;
            }

            if(color.b < color_range[INDEX_COLOR_MIN].b)
            {
                color_range[INDEX_COLOR_MIN].b = color.b;
            }
            else if(color.b > color_range[INDEX_COLOR_MAX].b)
            {
                color_range[INDEX_COLOR_MAX].b = color.b;
            }
        }
    }

    return color_range;
}

std::size_t Image::load(const char* file_name)
{
    reset();
    
    std::vector<unsigned char> png_buffer;
    lodepng::State state;

    std::size_t error = lodepng::load_file(png_buffer, file_name);

    if(!error)
    {
        error = lodepng::decode(pixels[0], width, height, state, png_buffer);
    }

    if(!error && frames > 1)
    {
        for(std::size_t index_frame = 1; index_frame < frames; index_frame++)
        {
            pixels[index_frame].resize(width * height * Color::NUM_BYTES);
        }
    }

    if(error)
    {
        std::cerr << "error: load " << file_name << " - " << error << ": " << lodepng_error_text(error) << std::endl;
        return error;
    }

    if(state.info_png.gama_defined)
    {
        gamma = 100000.0 / static_cast<double>(state.info_png.gama_gamma);
    }
    else
    {
        gamma = 2.2;
    }

    return error;
}

std::size_t Image::save(const char* file_name)
{
    std::string file_name_string = file_name;
    std::size_t error = 0;

    if(file_name_string.contains(".png"))
    {
        // save as a single frame png
        if(frames == 1)
        {
            error = lodepng::encode(file_name, pixels[0], width, height);
        }
        // save as a multi-frame png (sprite sheet)
        else if(frames > 1)
        {
            std::vector<unsigned char> combined;

            // combine each frame into a contiguous array
            for(std::size_t index_frame = 0; index_frame < frames; index_frame++)
            {
                combined.insert(combined.end(), pixels[index_frame].begin(), pixels[index_frame].end());
            }

            error = lodepng::encode(file_name, combined, width, height * frames);
        }

        if(error)
        {
            std::cout << "error: png save " << file_name << " - " << error << ": "<< lodepng_error_text(error) << std::endl;
        }
    }
    else if(file_name_string.contains(".gif"))
    {
        const uint32_t DELAY = 2; // many gif viewers will ignore delay values < 2
        const uint32_t BIT_DEPTH = 8;
        const bool DITHER = false;
        GifWriter writer = {};
        bool gif_save_success = GifBegin(&writer, file_name, width, height, DELAY, BIT_DEPTH, DITHER);

        for(std::size_t index_frame = 0; index_frame < frames; index_frame++)
        {
            gif_save_success = GifWriteFrame(&writer, pixels[index_frame].data(), width, height, DELAY, BIT_DEPTH, DITHER);
        }

        if(!gif_save_success)
        {
            std::cout << "error: gif save - " << file_name << std::endl;
            error = 1;
        }

        gif_save_success = GifEnd(&writer);

        if(!gif_save_success)
        {
            std::cout << "error: gif end - " << file_name << std::endl;
            error = 1;
        }
    }
    else
    {
        std::cout << "error: incorrect file format " << file_name << std::endl;
    }

    return error;
}
