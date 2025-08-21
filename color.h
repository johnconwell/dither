#ifndef __COLOR_H
#define __COLOR_H

#include <cstdint>

struct Color
{
    static inline constexpr int NUM_BYTES_COLOR = 4;
    static inline constexpr int INDEX_R = 0;
    static inline constexpr int INDEX_G = 1;
    static inline constexpr int INDEX_B = 2;
    static inline constexpr int INDEX_A = 3;

    static inline constexpr int CHANNEL_MAX = 255;

    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    Color();
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
};

#endif
