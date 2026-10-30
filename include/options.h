#pragma once

#include <array> // std::array
#include <cstddef> // std::size_t
#include <optional> // std::optional
#include <string> // std::string
#include <string_view> // std::string_view

// Enum-like command line arguments, parsed once in CLI and passed down as enums, so the
// per-pixel code never compares strings.

enum class MappingMethod
{
    EUCLIDEAN_DISTANCE,
    MANHATTAN_DISTANCE,
    UNIFORM_HISTOGRAM,
};

enum class ScanPattern
{
    RASTER,
    SERPENTINE,
};

enum class TemporalMethod
{
    // deterministic: each pixel shows the lighter colour in exactly round(duty * frames) frames,
    // starting at a blue-noise phase
    PWM,
    // each frame independently draws a random number against the duty cycle
    STOCHASTIC_PWM,
    RANDOM,
};

template <class E>
struct EnumName
{
    std::string_view name;
    E value;
};

// The command line spelling of every value, in the order they are listed in error messages.
template <class E>
struct EnumNames;

template <>
struct EnumNames<MappingMethod>
{
    static constexpr std::array<EnumName<MappingMethod>, 3> values = {{
        {"EUCLIDEAN_DISTANCE", MappingMethod::EUCLIDEAN_DISTANCE},
        {"MANHATTAN_DISTANCE", MappingMethod::MANHATTAN_DISTANCE},
        {"UNIFORM_HISTOGRAM", MappingMethod::UNIFORM_HISTOGRAM},
    }};
};

template <>
struct EnumNames<ScanPattern>
{
    static constexpr std::array<EnumName<ScanPattern>, 2> values = {{
        {"RASTER", ScanPattern::RASTER},
        {"SERPENTINE", ScanPattern::SERPENTINE},
    }};
};

template <>
struct EnumNames<TemporalMethod>
{
    static constexpr std::array<EnumName<TemporalMethod>, 3> values = {{
        {"PWM", TemporalMethod::PWM},
        {"STOCHASTIC_PWM", TemporalMethod::STOCHASTIC_PWM},
        {"RANDOM", TemporalMethod::RANDOM},
    }};
};

// "EUCLIDEAN_DISTANCE" -> MappingMethod::EUCLIDEAN_DISTANCE; std::nullopt for anything else
template <class E>
constexpr std::optional<E> parse_enum(std::string_view text)
{
    for(const auto& entry : EnumNames<E>::values)
    {
        if(entry.name == text)
        {
            return entry.value;
        }
    }

    return std::nullopt;
}

template <class E>
constexpr std::string_view enum_name(E value)
{
    for(const auto& entry : EnumNames<E>::values)
    {
        if(entry.value == value)
        {
            return entry.name;
        }
    }

    return "?";
}

// "A, B, C", for error messages
template <class E>
std::string enum_names()
{
    std::string output;

    for(const auto& entry : EnumNames<E>::values)
    {
        output += (output.empty() ? "" : ", ") + std::string(entry.name);
    }

    return output;
}
