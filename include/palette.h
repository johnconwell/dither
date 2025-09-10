#ifndef __PALETTE_H
#define __PALETTE_H

#include "color.h" // Color structure
#include <climits> // INT_MAX
#include <cmath> // std::abs()
#include <string> // std::string
#include <unordered_map> // std::unordered_map
#include <vector> // std::vector

enum PresetPalette
{
    BLACK_WHITE,
    _1BIT_MONITOR_GLOW,
    TITANSTONE,
    _2BIT_DEMICHROME,
    TWILIGHT5
};

class Palette
{
public:
    Palette();
    Palette(std::string name, std::vector<Color> colors);
    size_t size();
    Color get_color_at(int index);
    void add_color(Color color);
    void remove_color(Color color);
    void sort();
    Color nearest(Color color);
    Color nearest_grayscale(Color color);
    size_t nearest_index_lower(Color color);
    size_t pitch_scalar();
    Color pitch_vector();

    std::string to_string();

    static inline const std::vector<Color> BLACK_WHITE = {Color(0x000000ff), Color(0xffffffff)};
    static inline const std::vector<Color> _1BIT_MONITOR_GLOW = {Color(0x222323ff), Color(0xf0f6f0ff)};
    static inline const std::vector<Color> TITANSTONE = {Color(0x150413ff), Color(0x635c6dff), Color(0xffffd1ff)};
    static inline const std::vector<Color> _2BIT_DEMICHROME = {Color(0x211e20ff), Color(0x555568ff), Color(0xa0a08bff), Color(0xe9efecff)};
    static inline const std::vector<Color> TWILIGHT5 = {Color(0x292831ff), Color(0x333f58ff), Color(0x4a7a96ff), Color(0xee8695ff), Color(0xfbbbadff)};

    static const std::unordered_map<PresetPalette, std::vector<Color>> preset_palettes;

private:
    std::string name;
    std::vector<Color> colors;
};

#endif
