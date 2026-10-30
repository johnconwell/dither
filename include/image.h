#pragma once

#include "color.h"
#include <cstddef> // std::size_t
#include <string> // std::string
#include <vector> // std::vector

// One or more RGBA8 frames of the same size. Loads PNG; saves PNG (frames stacked into a
// vertical sprite sheet) or GIF (one frame each).
class Image
{
public:
    Color get_pixel(std::size_t x, std::size_t y, std::size_t frame = 0) const;
    void set_pixel(const Color& color, std::size_t x, std::size_t y, std::size_t frame = 0);
    std::size_t get_width() const;
    std::size_t get_height() const;
    // decoding exponent from a gAMA chunk, or 0 for the standard sRGB curve (see Transfer)
    double get_gamma() const;
    std::size_t get_frames() const;
    // new frames start out transparent black; frame 0 is kept
    void set_frames(std::size_t new_frames);

    // both throw DitherError on failure
    void load(const std::string& file_name);
    void save(const std::string& file_name) const;

private:
    std::vector<std::vector<unsigned char>> pixels = std::vector<std::vector<unsigned char>>(1);
    unsigned int width = 0;
    unsigned int height = 0;
    double gamma = 0.0;
};
