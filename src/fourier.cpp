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