#ifndef __FOURIER_H
#define __FOURIER_H

#include <cmath> // min(), max()
#include <cstddef> // size_t
#include <fftw3.h> // fft types + functions
#include <string> // std::string
#include <vector> // std::vector

class Fourier2D
{
public:
    Fourier2D();
    Fourier2D(std::vector<std::vector<int>> dataset, bool remove_dc_offset, bool center_output);
    std::vector<std::vector<int>> get_dataset();
    std::vector<std::vector<int>> get_transform();
    void dft();
    void idft();
    void normalize_dataset(size_t output_levels);
    void normalize_transform(size_t output_levels);

    std::string to_string();

private:
    void normalize(std::vector<std::vector<int>> &array, size_t output_levels);
    double magnitude(fftw_complex &value);

    std::vector<std::vector<int>> dataset;
    size_t height;
    size_t width;
    std::vector<std::vector<int>> transform;
    bool remove_dc_offset;
    bool center_output;
};

#endif
