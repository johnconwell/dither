#ifndef __GRAYSCALE_H
#define __GRAYSCALE_H

#include "color.h" // Color::NUM_BYTES_COLOR, Color::INDEX_R, Color::INDEX_G, Color::INDEX_B
#include <cstdint> // int16_t
#include <unordered_map> // std::unordered_map

enum GrayscaleMethod
{
    STANDARD,
    BT709,
    CHANNEL_R,
    CHANNEL_G,
    CHANNEL_B,
    CHANNEL_RG,
    CHANNEL_RB,
    CHANNEL_GB
};

struct Grayscale
{
    static int16_t channel_value(Color color, GrayscaleMethod method);

    static inline constexpr double STANDARD[Color::NUM_BYTES_COLOR] = {0.3334, 0.3333, 0.3333, 1.0};
    static inline constexpr double BT709[Color::NUM_BYTES_COLOR] = {0.2126, 0.7152, 0.0722, 1.0};
    static inline constexpr double CHANNEL_R[Color::NUM_BYTES_COLOR] = {1.0, 0.0, 0.0, 1.0};
    static inline constexpr double CHANNEL_G[Color::NUM_BYTES_COLOR] = {0.0, 1.0, 0.0, 1.0};
    static inline constexpr double CHANNEL_B[Color::NUM_BYTES_COLOR] = {0.0, 0.0, 1.0, 1.0};
    static inline constexpr double CHANNEL_RG[Color::NUM_BYTES_COLOR] = {0.5, 0.5, 0.0, 1.0};
    static inline constexpr double CHANNEL_RB[Color::NUM_BYTES_COLOR] = {0.5, 0.0, 0.5, 1.0};
    static inline constexpr double CHANNEL_GB[Color::NUM_BYTES_COLOR] = {0.0, 0.5, 0.5, 1.0};

    static const std::unordered_map<GrayscaleMethod, const double*> weights;
};


#endif
