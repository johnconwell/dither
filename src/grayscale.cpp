#include "grayscale.h"
#include "csv.h"
#include "error.h"
#include <filesystem> // std::filesystem::exists
#include <unordered_map> // std::unordered_map

namespace
{
    // built once, on first use, and shared
    const std::unordered_map<std::string, Grayscale::Weights>& presets()
    {
        static const std::unordered_map<std::string, Grayscale::Weights> PRESETS = {
            {"STANDARD", {0.3334, 0.3333, 0.3333}},
            {"BT709", {0.2126, 0.7152, 0.0722}},
            {"CHANNEL_R", {1.0, 0.0, 0.0}},
            {"CHANNEL_G", {0.0, 1.0, 0.0}},
            {"CHANNEL_B", {0.0, 0.0, 1.0}},
            {"CHANNEL_RG", {0.5, 0.5, 0.0}},
            {"CHANNEL_RB", {0.5, 0.0, 0.5}},
            {"CHANNEL_GB", {0.0, 0.5, 0.5}},
        };

        return PRESETS;
    }
}

Grayscale::Grayscale(const std::string& name) : name(name), weights{}
{
    if(const auto it = presets().find(name); it != presets().end())
    {
        weights = it->second;
        return;
    }

    if(!std::filesystem::exists(name))
    {
        throw DitherError("Invalid greyscale method - " + name);
    }

    const csv::Matrix matrix = csv::read_matrix(name, "grayscale weight");

    if(matrix.size() != 1 || matrix[0].size() != weights.size())
    {
        throw DitherError("grayscale weights file " + name + " should contain one row of " + std::to_string(weights.size()) + " values");
    }

    for(std::size_t index_channel = 0; index_channel < weights.size(); index_channel++)
    {
        weights[index_channel] = matrix[0][index_channel];
    }
}

bool Grayscale::has_preset(std::string_view name)
{
    return presets().contains(std::string(name));
}

std::vector<std::string> Grayscale::preset_names()
{
    std::vector<std::string> names;

    for(const auto& [name, _] : presets())
    {
        names.push_back(name);
    }

    return names;
}
