#ifndef __PALETTE_H
#define __PALETTE_H

#include "color.h" // Color structure
#include <climits> // INT_MAX
#include <limits> // std::numeric_limits
#include <cmath> // std::abs()
#include <cstdlib> // std::exit
#include <fstream> // std::ifstream
#include <iostream> // std::cerr, std::endl
#include <sstream> // std::stringstream
#include <string> // std::string
#include <unordered_map> // std::unordered_map
#include <vector> // std::vector

class Palette
{
public:
    const std::unordered_map<std::string, std::vector<Color>> PRESET_PALETTES =
    {
        {"1BIT_MONITOR_GLOW", {Color(0x222323), Color(0xf0f6f0)}},
        {"NOIRE_TRUTH", {Color(0x1e1c32), Color(0xc6baac)}},
        {"TITANSTONE", {Color(0x150413), Color(0x635c6d), Color(0xffffd1)}},
        {"ICE_CREAM_GB", {Color(0x7c3f58), Color(0xeb6b6f), Color(0xf9a875), Color(0xfff6d3)}},
        {"MIST_GB", {Color(0x2d1b00), Color(0x1e606e), Color(0x5ab9a8), Color(0xc4f0c2)}},
        {"CRIMSON", {Color(0x1b0326), Color(0x7a1c4b), Color(0xba5044), Color(0xeff9d6)}},
        {"FIERY_PLAGUE_GB", {Color(0x1a2129), Color(0x312137), Color(0x512839), Color(0x713141)}},
        {"TWILIGHT5", {Color(0x292831), Color(0x333f58), Color(0x4a7a96), Color(0xee8695), Color(0xfbbbad)}},
        {"OIL6", {Color(0x272744), Color(0x494d7e), Color(0x8b6d9c), Color(0xc69fa5), Color(0xf2d3ab), Color(0xfbf5ef)}},
        {"MIDNIGHT_ABLAZE", {Color(0x130208), Color(0x1f0510), Color(0x31051e), Color(0x460e2b), Color(0x7c183c), Color(0xd53c6a), Color(0xff8274)}},
        {"SLSO8", {Color(0x0d2b45), Color(0x203c56), Color(0x544e68), Color(0x8d697a), Color(0xd08159), Color(0xffaa5e), Color(0xffd4a3), Color(0xffecd6)}}
    };

    Palette() = delete;
    Palette(std::string name);

    // UNIFORM_HISTOGRAM: per-channel linear map that stretches the image's range onto the palette's
    // range, in working space. A channel that is flat in the image has slope UNMAPPED (NaN) and is
    // passed through unchanged (B4).
    struct RangeMap
    {
        RGBf input_min;
        RGBf output_min;
        RGBf slope;

        RGBf apply(const RGBf& c) const;
    };

    static inline const float UNMAPPED = std::numeric_limits<float>::quiet_NaN();

    std::size_t size() const;
    // original sRGB colour, which is what gets written to the output image
    Color get_color_at(std::size_t index) const;
    // the same colour in working space (linear light when gamma correction is on)
    const RGBf& get_working_at(std::size_t index) const;

    // Choose the working space. nullptr = sRGB values as float; otherwise the palette is
    // linearised once with this transfer function, so pixels and palette are compared in
    // the same space (B3). Call before sort() and the searches below.
    void set_working_space(const Transfer* transfer);

    // sort darkest to lightest by working-space HSL lightness
    void sort();

    // index of the nearest palette colour to a working-space value
    std::size_t nearest_index(const RGBf& input, const std::string& mapping_method, const RangeMap* range_map = nullptr) const;
    int nearest_index_lighter(const RGBf& input) const;
    int nearest_index_darker(const RGBf& input) const;

    // average absolute step between neighbouring colours (palette must be sorted), in working space
    RGBf pitch_vector() const;

    // UNIFORM_HISTOGRAM map from an image's working-space range {min, max} onto this palette's range
    RangeMap range_map(const RGBf& image_min, const RGBf& image_max) const;

    std::string to_string();

private:
    std::string name;
    std::vector<Color> colors;
    std::vector<RGBf> working;
};


#endif
