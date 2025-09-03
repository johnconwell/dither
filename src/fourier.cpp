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
std::vector<int> fourier_1d(std::vector<int> dataset)
{
    size_t size = dataset.size();

    double *in = fftw_alloc_real(size);
    fftw_complex *out = fftw_alloc_complex(size / 2 + 1);
    fftw_plan plan = fftw_plan_dft_r2c_1d(size, in, out, FFTW_ESTIMATE);

    // fill the in array with data from dataset
    for(size_t index_dataset = 0; index_dataset < size; index_dataset++)
    {
        in[index_dataset] = static_cast<double>(dataset[index_dataset]);
    }

    fftw_execute(plan);

    std::vector<int> transform = std::vector<int>(size, 0);

    // fill output vector with the results of the transform
    for(size_t index_transform = 0; index_transform < size / 2 + 1; index_transform++)
    {
        transform[index_transform] = static_cast<int>(sqrt(out[index_transform][0] * out[index_transform][0] + out[index_transform][1] * out[index_transform][1]));
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return transform;
}
