#ifndef __PALETTE_H
#define __PALETTE_H

#include "color.h"
#include <climits>
#include <string>
#include <unordered_map>
#include <vector>

enum PresetPalette
{
    _1BIT_MONITOR_GLOW,
    TWILIGHT5
};

class Palette
{
private:
    std::string name;
    std::vector<Color> colors;

public:
    static inline const std::vector<Color> _1BIT_MONITOR_GLOW = {Color(0x222323ff), Color(0xf0f6f0ff)};
    static inline const std::vector<Color> TWILIGHT5 = {Color(0x292831ff), Color(0x333f58ff), Color(0x4a7a96ff), Color(0xee8695ff), Color(0xfbbbadff)};

    static const std::unordered_map<PresetPalette, std::vector<Color>> preset_palettes;

    Palette();
    Palette(std::string name, std::vector<Color> colors);
    int size();
    Color get_color_at(int index);
    void add_color(Color color);
    void remove_color(Color color);
    void sort();
    Color nearest(Color color);

    std::string to_string();
};

#endif
