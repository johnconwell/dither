#ifndef __FOURIER_H
#define __FOURIER_H

#include <cmath>
#include <fftw3.h>
#include <vector>

void fourier();
std::vector<int> fourier_1d(std::vector<int> dataset, bool center_output);
std::vector<std::vector<int>> fourier_transform_2d(std::vector<std::vector<int>> dataset, bool remove_dc_offset, bool center_output);
std::vector<std::vector<int>> fourier_normalize_2d(std::vector<std::vector<int>> transform, int output_levels);

double magnitude(fftw_complex& value);

class Fourier
{
public:


private:

};

#endif
