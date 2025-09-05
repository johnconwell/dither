#include "fourier.h"
#include <iostream>

void fourier()
{
    int width = 10;
    int height = 10;
    // fftw_complex *in = fftw_alloc_complex(N);
    double *in = fftw_alloc_real(width * height);
    fftw_complex *out = fftw_alloc_complex(width * height);
    // fftw_plan p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan p1 = fftw_plan_dft_r2c_2d(height, width, in, out, FFTW_ESTIMATE);
    fftw_plan p2 = fftw_plan_dft_c2r_2d(height, width, out, in, FFTW_ESTIMATE);

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            in[(y * height) + x] = static_cast<double>(x ^ y);
            std::cout << in[(y * height) + x] << " ";
        }
        std::cout << std::endl;
    }

    fftw_execute(p1);

    out[5][0] = 0;
    out[5][1] = 0;
    out[6][0] = 0;
    out[6][1] = 0;
    out[7][0] = 0;
    out[7][1] = 0;
    out[8][0] = 0;
    out[8][1] = 0;

    fftw_execute(p2);

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            std::cout << out[(y * height) + x][0] << " ";
            std::cout << out[(y * height) + x][1] << "\t";
        }
        std::cout << std::endl;
    }

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            in[(y * height) + x] /= (width * height);
            std::cout << in[(y * height) + x] << " ";
        }
        std::cout << std::endl;
    }

    fftw_destroy_plan(p1);
    fftw_destroy_plan(p2);
    fftw_free(in);
    fftw_free(out);
}

// returns the fourier transform of the specified 1d dataset
std::vector<int> fourier_1d(std::vector<int> dataset, bool center_output)
{
    size_t size = dataset.size();

    fftw_complex* in = fftw_alloc_complex(size);
    fftw_complex* out = fftw_alloc_complex(size);
    fftw_plan plan = fftw_plan_dft_1d(size, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // fill the in array with data from dataset
    for(size_t index_dataset = 0; index_dataset < size; index_dataset++)
    {
        in[index_dataset][0] = static_cast<double>(dataset[index_dataset]);
        in[index_dataset][1] = 0.0; // no imaginary component
    }

    // center output
    if(center_output)
    {
        for(size_t index_dataset = 0; index_dataset < size; index_dataset++)
        {
            in[index_dataset][0] *= pow(-1, index_dataset);
        }
    }

    fftw_execute(plan);

    std::vector<int> transform = std::vector<int>(size, 0);

    // fill output vector with the results of the transform
    for(size_t index_transform = 0; index_transform < size; index_transform++)
    {
        transform[index_transform] = static_cast<int>(magnitude(out[index_transform]));
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return transform;
}

std::vector<std::vector<int>> fourier_transform_2d(std::vector<std::vector<int>> dataset, bool remove_dc_offset, bool center_output)
{
    size_t height = dataset.size();
    size_t width = dataset[0].size();
    size_t size = width * height;

    fftw_complex *in = fftw_alloc_complex(size);
    fftw_complex *out = fftw_alloc_complex(size);
    fftw_plan plan = fftw_plan_dft_2d(height, width, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // fill the in array with data from dataset
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            in[y * height + x][0] = static_cast<double>(dataset[y][x]);
            in[y * height + x][1] = 0.0;
        }
    }

    if(remove_dc_offset)
    {
        int mean = 0;

        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                mean += in[y * height + x][0];
            }
        }

        mean /= size;

        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                in[y * height + x][0] -= mean;
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
                in[y * height + x][0] *= pow(-1, y + x);
            }
        }
    }

    fftw_execute(plan);

    std::vector<std::vector<int>> transform = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));

    // fill output vector with the results of the transform
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            transform[y][x] = static_cast<int>(magnitude(out[y * height + x]));
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return transform;
}



double magnitude(fftw_complex &value)
{
    return sqrt(value[0] * value[0] + value[1] * value[1]);
}
