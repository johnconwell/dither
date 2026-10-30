#include "palette.h"
#include "error.h"
#include <algorithm> // std::min
#include <charconv> // std::from_chars
#include <cmath> // std::fabs, std::isnan
#include <fstream> // std::ifstream
#include <sstream> // std::stringstream
#include <unordered_map> // std::unordered_map

namespace
{
    // built once, on first use, and shared (ROADMAP 4.2)
    const std::unordered_map<std::string, std::vector<Color>>& presets()
    {
        static const std::unordered_map<std::string, std::vector<Color>> PRESETS = {
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
            {"SLSO8", {Color(0x0d2b45), Color(0x203c56), Color(0x544e68), Color(0x8d697a), Color(0xd08159), Color(0xffaa5e), Color(0xffd4a3), Color(0xffecd6)}},
        };

        return PRESETS;
    }
}

Palette::Palette(const std::string& name)
{
    if(const auto it = presets().find(name); it != presets().end())
    {
        this->name = name;
        this->colors = it->second;
    }
    else
    {
        std::ifstream file(name);

        if(!file.is_open())
        {
            throw DitherError("Invalid palette argument - " + name);
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
            throw DitherError("Palette file has no colors (expected name,author,color,...) - " + name);
        }

        this->name = tokens[0];

        for(std::size_t index_tokens = 2; index_tokens < tokens.size(); index_tokens++)
        {
            // tolerate spaces around a value
            std::string_view hex = tokens[index_tokens];
            hex.remove_prefix(std::min(hex.find_first_not_of(" \t"), hex.size()));
            hex.remove_suffix(hex.size() - std::min(hex.find_last_not_of(" \t") + 1, hex.size()));
            unsigned int hex_value = 0;
            const auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), hex_value, 16);

            if(hex.empty() || ec == std::errc::invalid_argument || ptr != hex.data() + hex.size())
            {
                throw DitherError("Invalid palette value - " + tokens[index_tokens]);
            }

            if(ec == std::errc::result_out_of_range || hex_value > Color::HEX_MAX)
            {
                throw DitherError("Palette value is out of color hex code range (0-ffffff) - " + tokens[index_tokens]);
            }

            colors.push_back(Color(hex_value));
        }
    }

    set_working_space(nullptr);
}

bool Palette::has_preset(std::string_view name)
{
    return presets().contains(std::string(name));
}

std::vector<std::string> Palette::preset_names()
{
    std::vector<std::string> names;

    for(const auto& [name, _] : presets())
    {
        names.push_back(name);
    }

    return names;
}

std::size_t Palette::size() const
{
    return colors.size();
}

Color Palette::get_color_at(std::size_t index) const
{
    return colors[index];
}

const RGBf& Palette::get_working_at(std::size_t index) const
{
    return working[index];
}

void Palette::set_working_space(const Transfer* transfer)
{
    working.clear();
    working.reserve(colors.size());

    for(const Color& color : colors)
    {
        working.push_back(transfer ? transfer->to_linear(color) : RGBf(color));
    }

    return;
}

void Palette::sort()
{
    const std::size_t colors_size = colors.size();

    for(std::size_t index_primary = 0; index_primary + 1 < colors_size; index_primary++)
    {
        for(std::size_t index_secondary = index_primary + 1; index_secondary < colors_size; index_secondary++)
        {
            if(working[index_secondary].lightness() < working[index_primary].lightness())
            {
                std::swap(colors[index_primary], colors[index_secondary]);
                std::swap(working[index_primary], working[index_secondary]);
            }
        }
    }

    return;
}

RGBf Palette::RangeMap::apply(const RGBf& c) const
{
    auto map_channel = [](float value, float channel_slope, float input_min, float output_min) {
        return std::isnan(channel_slope) ? value : output_min + channel_slope * (value - input_min);
    };

    return {
        map_channel(c.r, slope.r, input_min.r, output_min.r),
        map_channel(c.g, slope.g, input_min.g, output_min.g),
        map_channel(c.b, slope.b, input_min.b, output_min.b),
    };
}

std::size_t Palette::nearest_index(const RGBf& input, MappingMethod mapping_method, const RangeMap* range_map) const
{
    std::size_t index_nearest = 0;
    double distance_nearest = std::numeric_limits<double>::infinity();
    const std::size_t colors_size = working.size();

    switch(mapping_method)
    {
    case MappingMethod::EUCLIDEAN_DISTANCE:
    case MappingMethod::UNIFORM_HISTOGRAM:
    {
        const RGBf target = (mapping_method == MappingMethod::UNIFORM_HISTOGRAM && range_map) ? range_map->apply(input) : input;

        for(std::size_t index_palette = 0; index_palette < colors_size; index_palette++)
        {
            const double distance = target.distance_euclidean_squared(working[index_palette]);

            if(distance < distance_nearest)
            {
                distance_nearest = distance;
                index_nearest = index_palette;
            }
        }

        break;
    }
    case MappingMethod::MANHATTAN_DISTANCE:
        for(std::size_t index_palette = 0; index_palette < colors_size; index_palette++)
        {
            const double distance = input.distance_manhattan(working[index_palette]);

            if(distance < distance_nearest)
            {
                distance_nearest = distance;
                index_nearest = index_palette;
            }
        }

        break;
    }

    return index_nearest;
}

int Palette::nearest_index_lighter(const RGBf& input) const
{
    const double input_lightness = input.lightness();
    const int palette_size = static_cast<int>(working.size());

    // none of the colors in the palette are lighter, so return error value
    if(working[static_cast<std::size_t>(palette_size - 1)].lightness() <= input_lightness)
    {
        return -1;
    }

    for(int index_palette = palette_size - 2; index_palette >= 0; index_palette--)
    {
        // found the first darker color, so the next lighter color on the palette must be the nearest lighter palette color
        if(working[static_cast<std::size_t>(index_palette)].lightness() < input_lightness)
        {
            return index_palette + 1;
        }
    }

    // no darker colors found, so the nearest lighter color must be the first palette color
    return 0;
}

int Palette::nearest_index_darker(const RGBf& input) const
{
    const double input_lightness = input.lightness();
    const int palette_size = static_cast<int>(working.size());

    // none of the colors in the palette are darker, so return error value
    if(working[0].lightness() >= input_lightness)
    {
        return -1;
    }

    for(int index_palette = 1; index_palette < palette_size; index_palette++)
    {
        // found the first lighter color, so the next darker color on the palette must be the nearest darker palette color
        if(working[static_cast<std::size_t>(index_palette)].lightness() > input_lightness)
        {
            return index_palette - 1;
        }
    }

    // no lighter colors found, so the nearest darker color must be the last palette color
    return palette_size - 1;
}

RGBf Palette::pitch_vector() const
{
    const std::size_t colors_size = working.size();
    RGBf pitch;

    // a single colour has no spread, and dividing by colors_size - 1 would divide by zero
    if(colors_size < 2)
    {
        return pitch;
    }

    for(std::size_t index_colors = 0; index_colors + 1 < colors_size; index_colors++)
    {
        pitch.r += std::fabs(working[index_colors].r - working[index_colors + 1].r);
        pitch.g += std::fabs(working[index_colors].g - working[index_colors + 1].g);
        pitch.b += std::fabs(working[index_colors].b - working[index_colors + 1].b);
    }

    return pitch * (1.0f / static_cast<float>(colors_size - 1));
}

Palette::RangeMap Palette::range_map(const RGBf& image_min, const RGBf& image_max) const
{
    RGBf palette_min(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
    RGBf palette_max(-palette_min.r, -palette_min.g, -palette_min.b);

    for(const RGBf& c : working)
    {
        palette_min = {std::min(palette_min.r, c.r), std::min(palette_min.g, c.g), std::min(palette_min.b, c.b)};
        palette_max = {std::max(palette_max.r, c.r), std::max(palette_max.g, c.g), std::max(palette_max.b, c.b)};
    }

    auto channel_slope = [](float in_min, float in_max, float out_min, float out_max) {
        // flat channel: nothing to stretch, and dividing would be a divide by zero (B4)
        return in_max <= in_min ? UNMAPPED : (out_max - out_min) / (in_max - in_min);
    };

    return {
        image_min,
        palette_min,
        RGBf(channel_slope(image_min.r, image_max.r, palette_min.r, palette_max.r),
             channel_slope(image_min.g, image_max.g, palette_min.g, palette_max.g),
             channel_slope(image_min.b, image_max.b, palette_min.b, palette_max.b)),
    };
}

std::string Palette::to_string() const
{
    std::string output = name + "\n";
    std::size_t colors_size = colors.size();

    for(std::size_t index_palette = 0; index_palette < colors_size; index_palette++)
    {
        output += colors[index_palette].to_string_hex() + "\n";
    }

    return output;
}
