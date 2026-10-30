#include "error_diffusion.h"
#include "error.h"
#include <unordered_map> // std::unordered_map

namespace
{
    using Taps = std::vector<ErrorDiffusion::Tap>;

    // built once, on first use, and shared
    const std::unordered_map<std::string, Taps>& presets()
    {
        //                      x-2  x-1   x  x+1  x+2
        // row y   (current):              *   R    RR
        // row y+1           :  LLD  LD    D   RD   RRD
        // row y+2           : LLDD LDD   DD  RDD  RRDD
        static const std::unordered_map<std::string, Taps> PRESETS = {
            {"LINEAR", {
                {1, 0, 1.0},
            }},
            {"FLOYD_STEINBERG", {
                                                         {1, 0, 7.0 / 16.0},
                {-1, 1, 3.0 / 16.0}, {0, 1, 5.0 / 16.0}, {1, 1, 1.0 / 16.0},
            }},
            {"JARVIS_JUDICE_NINKE", {
                                                                                     {1, 0, 7.0 / 48.0}, {2, 0, 5.0 / 48.0},
                {-2, 1, 3.0 / 48.0}, {-1, 1, 5.0 / 48.0}, {0, 1, 7.0 / 48.0}, {1, 1, 5.0 / 48.0}, {2, 1, 3.0 / 48.0},
                {-2, 2, 1.0 / 48.0}, {-1, 2, 3.0 / 48.0}, {0, 2, 5.0 / 48.0}, {1, 2, 3.0 / 48.0}, {2, 2, 1.0 / 48.0},
            }},
            {"STUCKI", {
                                                                                     {1, 0, 8.0 / 42.0}, {2, 0, 4.0 / 42.0},
                {-2, 1, 2.0 / 42.0}, {-1, 1, 4.0 / 42.0}, {0, 1, 8.0 / 42.0}, {1, 1, 4.0 / 42.0}, {2, 1, 2.0 / 42.0},
                {-2, 2, 1.0 / 42.0}, {-1, 2, 2.0 / 42.0}, {0, 2, 4.0 / 42.0}, {1, 2, 2.0 / 42.0}, {2, 2, 1.0 / 42.0},
            }},
            // Atkinson deliberately diffuses only 3/4 of the error
            {"ATKINSON", {
                                                         {1, 0, 1.0 / 8.0}, {2, 0, 1.0 / 8.0},
                {-1, 1, 1.0 / 8.0}, {0, 1, 1.0 / 8.0}, {1, 1, 1.0 / 8.0},
                                    {0, 2, 1.0 / 8.0},
            }},
            {"BURKES", {
                                                                                     {1, 0, 8.0 / 32.0}, {2, 0, 4.0 / 32.0},
                {-2, 1, 2.0 / 32.0}, {-1, 1, 4.0 / 32.0}, {0, 1, 8.0 / 32.0}, {1, 1, 4.0 / 32.0}, {2, 1, 2.0 / 32.0},
            }},
            {"SIERRA", {
                                                                                     {1, 0, 5.0 / 32.0}, {2, 0, 3.0 / 32.0},
                {-2, 1, 2.0 / 32.0}, {-1, 1, 4.0 / 32.0}, {0, 1, 5.0 / 32.0}, {1, 1, 4.0 / 32.0}, {2, 1, 2.0 / 32.0},
                                     {-1, 2, 2.0 / 32.0}, {0, 2, 3.0 / 32.0}, {1, 2, 2.0 / 32.0},
            }},
            {"SIERRA_TWO_ROW", {
                                                                                     {1, 0, 4.0 / 16.0}, {2, 0, 3.0 / 16.0},
                {-2, 1, 1.0 / 16.0}, {-1, 1, 2.0 / 16.0}, {0, 1, 3.0 / 16.0}, {1, 1, 2.0 / 16.0}, {2, 1, 1.0 / 16.0},
            }},
            {"SIERRA_LITE", {
                                                         {1, 0, 2.0 / 4.0},
                {-1, 1, 1.0 / 4.0}, {0, 1, 1.0 / 4.0},
            }},
        };

        return PRESETS;
    }

    // alternative spellings that are also accepted, mapped to the canonical preset name
    const std::unordered_map<std::string, std::string>& aliases()
    {
        static const std::unordered_map<std::string, std::string> ALIASES = {
            {"JARVICE_JUDICE_NINKE", "JARVIS_JUDICE_NINKE"},
        };

        return ALIASES;
    }

    std::string canonical(const std::string& name)
    {
        const auto it = aliases().find(name);
        return it == aliases().end() ? name : it->second;
    }
}

ErrorDiffusion::ErrorDiffusion(const std::string& name) : name(canonical(name))
{
    const auto it = presets().find(this->name);

    if(it == presets().end())
    {
        throw DitherError("invalid error diffusion algorithm - " + name);
    }

    taps = it->second;
}

bool ErrorDiffusion::has_preset(std::string_view name)
{
    return presets().contains(canonical(std::string(name)));
}

std::vector<std::string> ErrorDiffusion::preset_names()
{
    std::vector<std::string> names;

    for(const auto& [name, _] : presets())
    {
        names.push_back(name);
    }

    return names;
}
