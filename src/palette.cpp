#include "palette.h"
#include <string>

const std::unordered_map<PresetPalette, std::vector<Color>> Palette::preset_palettes = {
    {PresetPalette::BLACK_WHITE, Palette::BLACK_WHITE},
    {PresetPalette::_1BIT_MONITOR_GLOW, Palette::_1BIT_MONITOR_GLOW},
    {PresetPalette::TITANSTONE, Palette::TITANSTONE},
    {PresetPalette::_2BIT_DEMICHROME, Palette::_2BIT_DEMICHROME},
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
    int distance = INT_MAX;
    int distance_nearest = INT_MAX;
    size_t colors_size = colors.size();

    for(size_t index_palette = 0; index_palette < colors_size; index_palette++)
    {
        distance = color.distance_grayscale(colors[index_palette]);

        if(distance < distance_nearest)
        {
            distance_nearest = distance;
            index_nearest = index_palette;
        }
    }

    return colors[index_nearest];
}

// returns the index of the palette color nearest to and darker than the specified color
size_t Palette::nearest_index_lower(Color color)
{
    size_t index_nearest = 0;
    int distance = INT_MAX;
    int distance_nearest = INT_MAX;
    size_t colors_size = colors.size();

    for(size_t index_palette = 0; index_palette < colors_size - 1; index_palette++)
    {
        if(colors[index_palette] < color)
        {
            distance = color.distance_squared(colors[index_palette]);

            if(distance < distance_nearest)
            {
                distance_nearest = distance;
                index_nearest = index_palette;
            }
        }
    }

    return index_nearest;
}

// returns the average distance between colors in the palette as a scalar
size_t Palette::pitch_scalar()
{
    size_t colors_size = colors.size();
    double distance = 0.0;

    for(size_t index_colors = 0; index_colors < colors_size - 1; index_colors++)
    {
        distance += Color::distance_between(colors[index_colors], colors[index_colors + 1]);
    }

    return distance / static_cast<double>(colors_size - 1);
}

// returns the average distance between colors in the palette as a color vector
Color Palette::pitch_vector()
{
    size_t colors_size = colors.size();
    Color distance_vector = Color(0, 0, 0, Color::CHANNEL_MAX);

    for(size_t index_colors = 0; index_colors < colors_size - 1; index_colors++)
    {
        distance_vector.r += std::abs(colors[index_colors].r - colors[index_colors + 1].r);
        distance_vector.g += std::abs(colors[index_colors].g - colors[index_colors + 1].g);
        distance_vector.b += std::abs(colors[index_colors].b - colors[index_colors + 1].b);
    }

    distance_vector.r /= static_cast<double>(colors_size - 1);
    distance_vector.g /= static_cast<double>(colors_size - 1);
    distance_vector.b /= static_cast<double>(colors_size - 1);

    return distance_vector;
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
