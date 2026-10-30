#ifndef __IMAGE_H
#define __IMAGE_H

#include "color.h"
#include "gif.h"
#include "lodepng.h"
#include <algorithm> // std::clamp, std::transform
#include <cctype> // std::tolower
#include <filesystem> // std::filesystem::path
#include <string> // std::string
#include <iostream> // std::err, std::endl
#include <vector> // std::vector

class Image
{
public:
    Image();
    Image(std::size_t frames);

    void reset();

    Color get_pixel(unsigned int x, unsigned int y, std::size_t frame = 0);
    void set_pixel(Color color, unsigned int x, unsigned int y, std::size_t frame = 0);
    std::size_t get_width();
    std::size_t get_height();
    // decoding exponent from a gAMA chunk, or 0 for the standard sRGB curve (see Transfer)
    double get_gamma();
    std::size_t get_frames();
    void set_frames(std::size_t new_frames);

    std::size_t load(const char* file_name);
    std::size_t save(const char* file_name);

private:
    std::vector<std::vector<unsigned char>> pixels;
    unsigned int width;
    unsigned int height;
    double gamma;
    std::size_t frames;
};

#endif
