#include "palette.h"
#include <string>

const std::unordered_map<PresetPalette, std::vector<Color>> Palette::preset_palettes = {
    {PresetPalette::BLACK_WHITE, Palette::BLACK_WHITE},
    {PresetPalette::_1BIT_MONITOR_GLOW, Palette::_1BIT_MONITOR_GLOW},
    {PresetPalette::TWILIGHT5, Palette::TWILIGHT5}
};

// initializes an empty palette
Palette::Palette()
{
    name = "";
    colors.resize(0);
    return;
}

// initializes an empty palette with a name
Palette::Palette(std::string name, std::vector<Color> colors)
{
    this->name = name;
    this->colors = colors;
    return;
}

// returns the size of the palette
size_t Palette::size()
{
    return colors.size();
}

// returns the color at the specified index
Color Palette::get_color_at(int index)
{
    return colors[index];
}

// adds a color to the palette
void Palette::add_color(Color color)
{
    colors.push_back(color);
    return;
}

// removes a color from the palette
void Palette::remove_color(Color color)
{
    size_t colors_size = colors.size();

    for(size_t index = 0; index < colors_size; index++)
    {
        if(colors[index] == color)
        {
            colors.erase(colors.begin() + index);
            return;
        }
    }

    return;
}

// sorts the palette from darkest colors (closest to black) to lightest colors (closest to white)
void Palette::sort()
{
    size_t colors_size = colors.size();

    for(size_t index_primary = 0; index_primary < colors_size - 1; index_primary++)
    {
        for(size_t index_secondary = index_primary + 1; index_secondary < colors_size; index_secondary++)
        {
            if(colors[index_secondary] < colors[index_primary])
            {
                Color temp = colors[index_primary];
                colors[index_primary] = colors[index_secondary];
                colors[index_secondary] = temp;
            }
        }
    }

    return;
}

// returns the palette color nearest to the specified color, using euclidean distance
Color Palette::nearest(Color color)
{
    int index_nearest = -1;
    int distance_squared_nearest = INT_MAX;
    size_t colors_size = colors.size();

    for(size_t index_palette = 0; index_palette < colors_size; index_palette++)
    {
        int distance_squared = color.distance_squared(colors[index_palette]);
        if(distance_squared < distance_squared_nearest)
        {
            distance_squared_nearest = distance_squared;
            index_nearest = index_palette;
        }
    }

    return colors[index_nearest];
}

// returns the palette color nearest to the specified color, assuming both colors are grayscale
Color Palette::nearest_grayscale(Color color)
{
    int index_nearest = -1;
    int distance_nearest = INT_MAX;
    size_t colors_size = colors.size();

    for(size_t index_palette = 0; index_palette < colors_size; index_palette++)
    {
        int distance = color.distance_grayscale(colors[index_palette]);
        if(distance < distance_nearest)
        {
            distance_nearest = distance;
            index_nearest = index_palette;
        }
    }

    return colors[index_nearest];
}

// returns a string representation of the palette
std::string Palette::to_string()
{
    std::string output = "";
    size_t colors_size = colors.size();

    for(size_t index_palette = 0; index_palette < colors_size; index_palette++)
    {
        output += colors[index_palette].to_string() + "\n";
    }

    return output;
}
