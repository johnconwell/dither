#pragma once

#include "color.h" // Color, RGBf, Transfer
#include "options.h" // MappingMethod
#include <string> // std::string
#include <string_view> // std::string_view
#include <vector> // std::vector

// Palette: a built-in preset by name, or a Lospec csv file (name,author,color,color,...).
class Palette
{
public:
    // throws DitherError for an unknown preset or an invalid file
    explicit Palette(const std::string& name);

    static bool has_preset(std::string_view name);
    static std::vector<std::string> preset_names();

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

    // index of the nearest palette colour to a working-space value. UNIFORM_HISTOGRAM is not a
    // per-pixel distance (Dither maps it through the image's luminance histogram, ROADMAP 5.1),
    // so here it falls back to EUCLIDEAN_DISTANCE.
    std::size_t nearest_index(const RGBf& input, MappingMethod mapping_method) const;
    int nearest_index_lighter(const RGBf& input) const;
    int nearest_index_darker(const RGBf& input) const;

    // average absolute step between neighbouring colours (palette must be sorted), in working space
    RGBf pitch_vector() const;

    std::string to_string() const;

private:
    std::string name;
    std::vector<Color> colors;
    std::vector<RGBf> working;
};
