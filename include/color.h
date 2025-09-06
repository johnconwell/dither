#ifndef __COLOR_H
#define __COLOR_H

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

struct Color
{
    Color();
    Color(int16_t r, int16_t g, int16_t b, int16_t a);
    Color(uint32_t hex_code);
    bool operator==(const Color& other) const;
    bool operator<(const Color& other) const;
    Color operator-(const Color& other) const;
    Color operator+(const Color& other) const;
    Color operator*(const double scalar) const;
    uint32_t hex();

    int distance_squared(Color other);
    double distance(Color other);
    int distance_redmean_squared(Color other);
    double distance_redmean(Color other);
    int distance_grayscale(Color other);

    std::string to_string();

    static inline constexpr int NUM_BYTES_COLOR = 4;
    static inline constexpr int INDEX_R = 0;
    static inline constexpr int INDEX_G = 1;
    static inline constexpr int INDEX_B = 2;
    static inline constexpr int INDEX_A = 3;

    static inline constexpr int CHANNEL_MAX = 255;

    int16_t r;
    int16_t g;
    int16_t b;
    int16_t a;
};

#endif
