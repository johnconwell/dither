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

        // tolerate Windows line endings
        if(!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::stringstream ss(line);
        
        while(std::getline(ss, token, ','))
        {
            tokens.push_back(token);
        }

        // Lospec format: name,author,color,color,...
        if(tokens.size() < 3)
        {
            std::cerr << "Error: Palette file has no colors (expected name,author,color,...) - " << name << std::endl;
            std::exit(EXIT_FAILURE);
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

    for(std::size_t index_primary = 0; index_primary + 1 < colors_size; index_primary++)
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
        // a channel with no range in the image (slope = UNMAPPED) passes straight through to the nearest-colour search
        auto map_channel = [&](int value, std::size_t index_channel, int input_min, int output_min) {
            const double channel_slope = slope[index_channel];
            return std::isnan(channel_slope) ? value : static_cast<int>(output_min + channel_slope * (value - input_min));
        };
        const int color_mapped_r = map_channel(input.r, Color::INDEX_R, input_range[INDEX_COLOR_MIN].r, output_range[INDEX_COLOR_MIN].r);
        const int color_mapped_g = map_channel(input.g, Color::INDEX_G, input_range[INDEX_COLOR_MIN].g, output_range[INDEX_COLOR_MIN].g);
        const int color_mapped_b = map_channel(input.b, Color::INDEX_B, input_range[INDEX_COLOR_MIN].b, output_range[INDEX_COLOR_MIN].b);
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

    // a single colour has no spread, and dividing by colors_size - 1 would divide by zero
    if(colors_size < 2)
    {
        return distance_vector;
    }

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

    for(std::size_t index_palette = 0; index_palette < palette_size; index_palette++)
    {
        const Color color = colors[index_palette];

        color.expand_range(color_range);
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

std::vector<double> Palette::range_slope(const std::vector<Color>& image_range, const std::vector<Color>& palette_range)
{
    const std::size_t INDEX_COLOR_MIN = 0;
    const std::size_t INDEX_COLOR_MAX = 1;

    auto channel_slope = [](int image_min, int image_max, int palette_min, int palette_max) {
        // flat channel: nothing to stretch, and dividing would be a divide by zero (B4)
        if(image_max <= image_min)
        {
            return UNMAPPED;
        }

        return static_cast<double>(palette_max - palette_min) / static_cast<double>(image_max - image_min);
    };

    return {
        channel_slope(image_range[INDEX_COLOR_MIN].r, image_range[INDEX_COLOR_MAX].r, palette_range[INDEX_COLOR_MIN].r, palette_range[INDEX_COLOR_MAX].r),
        channel_slope(image_range[INDEX_COLOR_MIN].g, image_range[INDEX_COLOR_MAX].g, palette_range[INDEX_COLOR_MIN].g, palette_range[INDEX_COLOR_MAX].g),
        channel_slope(image_range[INDEX_COLOR_MIN].b, image_range[INDEX_COLOR_MAX].b, palette_range[INDEX_COLOR_MIN].b, palette_range[INDEX_COLOR_MAX].b),
    };
}
