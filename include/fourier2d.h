#ifndef __FOURIER2D_H
#define __FOURIER2D_H

#include <cfloat> // DBL_MIN, DBL_MAX
#include <cmath> // min(), max()
#include <cstddef> // size_t
#include <fftw3.h> // fft types + functions
#include <string> // std::string
#include <type_traits> // std::is_integral
#include <vector> // std::vector

template <typename T>
class Fourier2D
{
public:
    Fourier2D() = delete;
    Fourier2D(std::vector<std::vector<T>> dataset, bool remove_dc_offset, bool center_output);
    std::vector<std::vector<T>> get_transform();
    std::vector<std::vector<T>> get_inverse();
    void dft();
    void idft();
    void normalize_transform(size_t output_levels = 2);
    void normalize_inverse(size_t output_levels = 2);

    std::string to_string();

private:
    void dft_raw();
    void dft_conditioned();
    void normalize(std::vector<std::vector<T>> &array, size_t output_levels);
    double magnitude(fftw_complex &value);

    size_t height;
    size_t width;
    std::vector<std::vector<T>> dataset_raw;
    std::vector<std::vector<T>> dataset_conditioned;
    fftw_complex* transform_complex;
    std::vector<std::vector<T>> transform_real;
    std::vector<std::vector<T>> inverse;
    bool remove_dc_offset;
    bool center_output;
};

template <typename T>
Fourier2D<T>::Fourier2D(std::vector<std::vector<T>> dataset, bool remove_dc_offset, bool center_output)
{
    this->height = dataset.size();
    this->width = dataset[0].size();
    this->dataset_raw = dataset;
    this->dataset_conditioned = std::vector<std::vector<T>>(height, std::vector<T>(width, static_cast<T>(0)));

    // copy raw data into new conditioned array
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            dataset_conditioned[y][x] = dataset_raw[y][x];
        }
    }

    // remove DC offset by subtracting the mean of the data
    if(remove_dc_offset)
    {
        double mean = 0;

        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                mean += dataset_raw[y][x];
            }
        }

        mean /= (height * width);

        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                dataset_conditioned[y][x] -= mean;
            }
        }
    }

    // center output
    if(center_output)
    {
        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                dataset_conditioned[y][x] *= pow(-1, y + x);
            }
        }
    }

    this->transform_complex = fftw_alloc_complex(height * width);
    this->transform_real = std::vector<std::vector<T>>(height, std::vector<T>(width, static_cast<T>(0)));
    this->inverse = std::vector<std::vector<T>>(height, std::vector<T>(width, static_cast<T>(0)));

    return;
}

template <typename T>
std::vector<std::vector<T>> Fourier2D<T>::get_transform()
{
    return transform_real;
}

template <typename T>
std::vector<std::vector<T>> Fourier2D<T>::get_inverse()
{
    return inverse;
}

template <typename T>
void Fourier2D<T>::dft()
{
    dft_raw(); // raw dft produced transform_complex, needed for idft
    dft_conditioned(); // conditioned dft produces transform_real, needed for visual depiction
    return;
}

template <typename T>
void Fourier2D<T>::idft()
{
    size_t size = width * height;

    fftw_complex *in = fftw_alloc_complex(size);
    fftw_complex *out = fftw_alloc_complex(size);
    fftw_plan plan = fftw_plan_dft_2d(height, width, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);

    // fill the in array with data from dataset
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            in[y * height + x][0] = transform_complex[y * height + x][0];
            in[y * height + x][1] = transform_complex[y * height + x][1];
        }
    }

    fftw_execute(plan);

    // fill output vector with the results of the transform
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            // results of idft are scaled by width * height, so divide to normalize
            inverse[y][x] = static_cast<T>(magnitude(out[y * height + x]) / static_cast<double>(size));
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return;
}

template <typename T>
void Fourier2D<T>::normalize_transform(size_t output_levels)
{
    normalize(transform_real, output_levels);
    return;
}

template <typename T>
void Fourier2D<T>::normalize_inverse(size_t output_levels)
{
    normalize(inverse, output_levels);
    return;
}

template <typename T>
std::string Fourier2D<T>::to_string()
{
    std::string output = "";

    output += "Dataset:\n";
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(dataset_raw[y][x]) + " ";
        }
        
        output += "\n";
    }

    output += "Transform:\n";
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(transform_real[y][x]) + " ";
        }
        
        output += "\n";
    }

    output += "Inverse:\n";
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(inverse[y][x]) + " ";
        }
        
        output += "\n";
    }

    return output;
}

template <typename T>
void Fourier2D<T>::dft_raw()
{
    size_t size = width * height;

    fftw_complex *in = fftw_alloc_complex(size);
    fftw_complex *out = fftw_alloc_complex(size);
    fftw_plan plan = fftw_plan_dft_2d(height, width, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // fill the in array with data from dataset
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            in[y * height + x][0] = static_cast<double>(dataset_raw[y][x]);
            in[y * height + x][1] = 0.0;
        }
    }

    fftw_execute(plan);

    // fill output vector with the results of the transform
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            transform_complex[y * height + x][0] = out[y * height + x][0];
            transform_complex[y * height + x][1] = out[y * height + x][1];
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return;
}

template <typename T>
void Fourier2D<T>::dft_conditioned()
{
    size_t size = width * height;

    fftw_complex *in = fftw_alloc_complex(size);
    fftw_complex *out = fftw_alloc_complex(size);
    fftw_plan plan = fftw_plan_dft_2d(height, width, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // fill the in array with data from dataset
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            in[y * height + x][0] = static_cast<double>(dataset_conditioned[y][x]);
            in[y * height + x][1] = 0.0;
        }
    }

    fftw_execute(plan);

    // fill output vector with the results of the transform
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            transform_real[y][x] = static_cast<T>(magnitude(out[y * height + x]));
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return;
}

template <typename T>
void Fourier2D<T>::normalize(std::vector<std::vector<T>> &array, size_t output_levels)
{
    size_t output_levels_clamped = std::max(output_levels, static_cast<size_t>(2));
    double array_min = DBL_MAX;
    double array_max = DBL_MIN;

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            array_min = std::min(array_min, static_cast<double>(array[y][x]));
            array_max = std::max(array_max, static_cast<double>(array[y][x]));
        }
    }

    double array_range = array_max - array_min;

    // normalize to output levels
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            array[y][x] = static_cast<T>((static_cast<double>(array[y][x]) - array_min) * static_cast<double>(output_levels_clamped - 1) / array_range);
        }
    }

    return;
}

template <typename T>
double Fourier2D<T>::magnitude(fftw_complex &value)
{
    return sqrt(value[0] * value[0] + value[1] * value[1]);
}

#endif
