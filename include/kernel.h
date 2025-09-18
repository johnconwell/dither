#ifndef __KERNEL_H
#define __KERNEL_H

#include <vector> // std::vector

enum Kernel
{
    RIDGE_4,
    RIDGE_8,
    SHARPEN_4,
    SHARPEN_8,
    BOX_BLUR,
    GAUSSIAN_BLUR,
    UNSHARP_MASK
};

static const std::unordered_map<Kernel, std::string, EnumHash> KERNEL_STRING = {
    {Kernel::RIDGE_4, "ridge_4"},
    {Kernel::RIDGE_8, "ridge_8"},
    {Kernel::SHARPEN_4, "sharpen_4"},
    {Kernel::SHARPEN_8, "sharpen_8"},
    {Kernel::BOX_BLUR, "box_blur"},
    {Kernel::GAUSSIAN_BLUR, "gaussian_blur"},
    {Kernel::UNSHARP_MASK, "unsharp_mask"}
};

const std::vector<std::vector<double>> KERNEL_RIDGE_4 = {
    {+0.0, -1.0, +0.0},
    {-1.0, +4.0, -1.0},
    {+0.0, -1.0, +0.0}
};

const std::vector<std::vector<double>> KERNEL_RIDGE_8 = {
    {-1.0, -1.0, -1.0},
    {-1.0, +8.0, -1.0},
    {-1.0, -1.0, -1.0}
};

const std::vector<std::vector<double>> KERNEL_SHARPEN_4 = {
    {+0.0, -1.0, +0.0},
    {-1.0, +5.0, -1.0},
    {+0.0, -1.0, +0.0}
};

const std::vector<std::vector<double>> KERNEL_SHARPEN_8 = {
    {-1.0, -1.0, -1.0},
    {-1.0, +9.0, -1.0},
    {-1.0, -1.0, -1.0}
};

const std::vector<std::vector<double>> KERNEL_BOX_BLUR = {
    {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
    {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
    {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0}
};

const std::vector<std::vector<double>> KERNEL_GAUSSIAN_BLUR = {
    {0.00296902, 0.0133062, 0.0219382, 0.0133062, 0.00296902},
    {0.0133062, 0.0596343, 0.0983203, 0.0596343, 0.0133062},
    {0.0219382, 0.0983203, 0.162103, 0.0983203, 0.0219382},
    {0.0133062, 0.0596343, 0.0983203, 0.0596343, 0.0133062},
    {0.00296902, 0.0133062, 0.0219382, 0.0133062, 0.00296902}
};

const std::vector<std::vector<double>> KERNEL_UNSHARP_MASK = {
    {-0.00296902, -0.0133062, -0.0219382, -0.0133062, -0.00296902},
    {-0.0133062, -0.0596343, -0.0983203, -0.0596343, -0.0133062},
    {-0.0219382, -0.0983203, +1.837897, -0.0983203, -0.0219382},
    {-0.0133062, -0.0596343, -0.0983203, -0.0596343, -0.0133062},
    {-0.00296902, -0.0133062, -0.0219382, -0.0133062, -0.00296902}
};

#endif
