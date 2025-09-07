#ifndef __ORDERED_H
#define __ORDERED_H

#include "helper.h" // EnumHash
#include <string> // std::string
#include <unordered_map> // std::unordered_map

enum ThresholdMatrix
{
    BAYER_2X2,
    BAYER_4X4,
    BAYER_8X8,
    BAYER_16X16,
    BAYER_32X32,
    BAYER_64X64,
    BLUE_NOISE_2X2,
    BLUE_NOISE_4X4,
    BLUE_NOISE_8X8,
    BLUE_NOISE_16X16,
    BLUE_NOISE_32X32,
    BLUE_NOISE_64X64,
    BROWN_NOISE_2X2,
    BROWN_NOISE_4X4,
    BROWN_NOISE_8X8,
    BROWN_NOISE_16X16,
    BROWN_NOISE_32X32,
    BROWN_NOISE_64X64,
    WHITE_NOISE_2X2,
    WHITE_NOISE_4X4,
    WHITE_NOISE_8X8,
    WHITE_NOISE_16X16,
    WHITE_NOISE_32X32,
    WHITE_NOISE_64X64
};

static const std::unordered_map<ThresholdMatrix, std::string, EnumHash> THRESHOLD_MATRIX_STRING = {
    {ThresholdMatrix::BAYER_2X2, "bayer_2x2"},
    {ThresholdMatrix::BAYER_4X4, "bayer_4x4"},
    {ThresholdMatrix::BAYER_8X8, "bayer_8x8"},
    {ThresholdMatrix::BAYER_16X16, "bayer_16x16"},
    {ThresholdMatrix::BAYER_32X32, "bayer_32x32"},
    {ThresholdMatrix::BAYER_64X64, "bayer_64x64"},
    {ThresholdMatrix::BLUE_NOISE_2X2, "blue_noise_2x2"},
    {ThresholdMatrix::BLUE_NOISE_4X4, "blue_noise_4x4"},
    {ThresholdMatrix::BLUE_NOISE_8X8, "blue_noise_8x8"},
    {ThresholdMatrix::BLUE_NOISE_16X16, "blue_noise_16x16"},
    {ThresholdMatrix::BLUE_NOISE_32X32, "blue_noise_32x32"},
    {ThresholdMatrix::BLUE_NOISE_64X64, "blue_noise_64x64"},
    {ThresholdMatrix::BROWN_NOISE_2X2, "brown_noise_2x2"},
    {ThresholdMatrix::BROWN_NOISE_4X4, "brown_noise_4x4"},
    {ThresholdMatrix::BROWN_NOISE_8X8, "brown_noise_8x8"},
    {ThresholdMatrix::BROWN_NOISE_16X16, "brown_noise_16x16"},
    {ThresholdMatrix::BROWN_NOISE_32X32, "brown_noise_32x32"},
    {ThresholdMatrix::BROWN_NOISE_64X64, "brown_noise_64x64"},
    {ThresholdMatrix::WHITE_NOISE_2X2, "white_noise_2x2"},
    {ThresholdMatrix::WHITE_NOISE_4X4, "white_noise_4x4"},
    {ThresholdMatrix::WHITE_NOISE_8X8, "white_noise_8x8"},
    {ThresholdMatrix::WHITE_NOISE_16X16, "white_noise_16x16"},
    {ThresholdMatrix::WHITE_NOISE_32X32, "white_noise_32x32"},
    {ThresholdMatrix::WHITE_NOISE_64X64, "white_noise_64x64"}
};

#endif
