#ifndef __KERNEL_H
#define __KERNEL_H

#include <vector> // std::vector

const std::vector<std::vector<double>> KERNEL_RIDGE_4 = {
    {+0.00, -0.25, +0.00},
    {-0.25, +1.00, -0.25},
    {+0.00, -0.25, +0.00}
};

const std::vector<std::vector<double>> KERNEL_RIDGE_8 = {
    {-0.125, -0.125, -0.125},
    {-0.125, +1.000, -0.125},
    {-0.125, -0.125, -0.125}
};

const std::vector<std::vector<double>> KERNEL_SHARPEN_4 = {
    {+0.00, -0.25, +0.00},
    {-0.25, +1.25, -0.25},
    {+0.00, -0.25, +0.00}
};

const std::vector<std::vector<double>> KERNEL_SHARPEN_8 = {
    {-0.125, -0.125, -0.125},
    {-0.125, +1.250, -0.125},
    {-0.125, -0.125, -0.125}
};

const std::vector<std::vector<double>> KERNEL_BOX_BLUR = {
    {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
    {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
    {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0}
};


#endif
