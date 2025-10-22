#include "palette.h"

Palette::Palette(std::string name)
{
    if(PRESET_PALETTES.contains(name))
    {
        this->name = name;
        this->colors = PRESET_PALETTES.at(name);
    }
    else
    {
        std::ifstream file;
        file.open(name);
        
        if(!file.is_open())
        {
            std::cerr << "Error: Invalid palette argument - " << name << std::endl;
            std::exit(EXIT_FAILURE);
        }

        std::string line;
        std::vector<std::string> tokens;
        std::string token;

        std::getline(file, line);
        std::stringstream ss(line);
        
        while(std::getline(ss, token, ','))
        {
            tokens.push_back(token);
        }

        this->name = tokens[0];

        for(std::size_t index_tokens = 2; index_tokens < tokens.size(); index_tokens++)
        {
            int hex_value;

            try
            {
                hex_value = std::stoi(tokens[index_tokens], nullptr, 16);
            }
            catch(const std::exception& e)
            {
                std::cerr << "Error: Invalid palette value - " << tokens[index_tokens] << std::endl;
                std::exit(EXIT_FAILURE);
            }

            if(hex_value < 0 || hex_value > static_cast<int>(Color::HEX_MAX))
            {
                std::cerr << "Error: Palette value is out of color hex code range (0-ffffff) - " << hex_value << std::endl;
                std::exit(EXIT_FAILURE);
            }
            
            colors.push_back(Color(hex_value));
        }
    }

    return;
}

std::size_t Palette::size()
{
    return colors.size();
}

Color Palette::get_color_at(std::size_t index)
{
    return colors[index];
}

void Palette::sort()
{
    const std::size_t colors_size = colors.size();

    for(std::size_t index_primary = 0; index_primary < colors_size - 1; index_primary++)
    {
        for(std::size_t index_secondary = index_primary + 1; index_secondary < colors_size; index_secondary++)
        {
            if(colors[index_secondary].get_lightness() < colors[index_primary].get_lightness())
            {
                Color temp = colors[index_primary];
                colors[index_primary] = colors[index_secondary];
                colors[index_secondary] = temp;
            }
        }
    }

    return;
}

Color Palette::nearest(Color input, std::string mapping_method, std::vector<Color> input_range, std::vector<Color> output_range, std::vector<double> slope)
{
    int index_nearest = -1;
    int distance_squared_nearest = INT_MAX;
    const std::size_t colors_size = colors.size();

    if(mapping_method == "EUCLIDEAN_DISTANCE")
    {
        for(std::size_t index_palette = 0; index_palette < colors_size; index_palette++)
        {
            const int distance_squared = input.distance_euclidean_squared(colors[index_palette]);
            
            if(distance_squared < distance_squared_nearest)
            {
                distance_squared_nearest = distance_squared;
                index_nearest = index_palette;
            }
        }
    }
    else if(mapping_method == "MANHATTAN_DISTANCE")
    {
        for(std::size_t index_palette = 0; index_palette < colors_size; index_palette++)
        {
            const int distance_squared = input.distance_manhattan(colors[index_palette]);

            if(distance_squared < distance_squared_nearest)
            {
                distance_squared_nearest = distance_squared;
                index_nearest = index_palette;
            }
        }
    }
    else if(mapping_method == "UNIFORM_HISTOGRAM")
    {
        const std::size_t INDEX_COLOR_MIN = 0;
        const int color_mapped_r = output_range[INDEX_COLOR_MIN].r + (slope[Color::INDEX_R] * (input.r - input_range[INDEX_COLOR_MIN].r));
        const int color_mapped_g = output_range[INDEX_COLOR_MIN].g + (slope[Color::INDEX_R] * (input.g - input_range[INDEX_COLOR_MIN].g));
        const int color_mapped_b = output_range[INDEX_COLOR_MIN].b + (slope[Color::INDEX_R] * (input.b - input_range[INDEX_COLOR_MIN].b));
        Color color_mapped = Color(color_mapped_r, color_mapped_g, color_mapped_b, Color::CHANNEL_MAX);

        for(std::size_t index_palette = 0; index_palette < colors_size; index_palette++)
        {
            const int distance_squared = color_mapped.distance_euclidean_squared(colors[index_palette]);

            if(distance_squared < distance_squared_nearest)
            {
                distance_squared_nearest = distance_squared;
                index_nearest = index_palette;
            }
        }
    }
    else
    {
        std::cerr << "Error: Invalid palette mapping method - " << mapping_method << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return colors[index_nearest];
}

int Palette::nearest_index_lighter(Color input)
{
    double input_lightness = input.get_lightness();
    std::size_t palette_size = colors.size();

    // none of the colors in the palette are lighter, so return error value
    if(colors[palette_size - 1].get_lightness() <= input_lightness)
    {
        return -1;
    }

    for(int index_palette = palette_size - 2; index_palette >= 0; index_palette--)
    {
        // found the first darker color, so the next lighter color on the palette must be the nearest lighter palette color
        if(colors[index_palette].get_lightness() < input_lightness)
        {
            return index_palette + 1;
        }
    }

    // no lighter colors found, so the nearest lighter color must be the last palette color
    return palette_size - 1;
}

int Palette::nearest_index_darker(Color input)
{
    double input_lightness = input.get_lightness();
    std::size_t palette_size = colors.size();

    // none of the colors in the palette are darker, so return error value
    if(colors[0].get_lightness() >= input_lightness)
    {
        return -1;
    }

    for(int index_palette = 1; index_palette < static_cast<int>(palette_size); index_palette++)
    {
        // found the first lighter color, so the next darker color on the palette must be the nearest darker palette color
        if(colors[index_palette].get_lightness() > input_lightness)
        {
            return index_palette - 1;
        }
    }

    // no darker colors found, so the nearest darker color must be the first palette color
    return 0;
}

Color Palette::pitch_vector()
{
    const std::size_t colors_size = colors.size();
    Color distance_vector = Color(0, 0, 0, Color::CHANNEL_MAX);

    for(std::size_t index_colors = 0; index_colors < colors_size - 1; index_colors++)
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

std::vector<Color> Palette::get_color_range()
{
    std::vector<Color> color_range = {
        Color(Color::CHANNEL_MAX, Color::CHANNEL_MAX, Color::CHANNEL_MAX, Color::CHANNEL_MAX),
        Color(0, 0, 0, Color::CHANNEL_MAX)
    };
    const std::size_t palette_size = colors.size();
    const std::size_t INDEX_COLOR_MIN = 0;
    const std::size_t INDEX_COLOR_MAX = 1;

    for(std::size_t index_palette = 0; index_palette < palette_size; index_palette++)
    {
        const Color color = colors[index_palette];

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

    return color_range;
}

std::string Palette::to_string()
{
    std::string output = name + "\n";
    std::size_t colors_size = colors.size();

    for(std::size_t index_palette = 0; index_palette < colors_size; index_palette++)
    {
        output += colors[index_palette].to_string_hex() + "\n";
    }

    return output;
}
