#include "fourier.h"

void fourier()
{
    fftw_complex *in;
    fftw_complex *out;
    fftw_plan p;
    int N = 1;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);


    

    fftw_execute(p); /* repeat as needed */
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
}