#ifndef __DITHER_H
#define __DITHER_H

#include "error.h"
#include "grayscale.h"
#include "image.h"
#include "lodepng.h"
#include "ordered.h"
#include "palette.h"
#include <cstddef>
#include <iostream>
#include <vector>




class Dither
{
private:
    Image image;
    Palette palette;

    void error_diffusion_standard(ErrorDiffusionAlgorithm algorithm);
    void error_diffusion_alternate(ErrorDiffusionAlgorithm algorithm);

public:
    Dither();
    void set_palette(Palette palette);
    size_t load(const char* file_name);
    size_t save(const char* file_name);
    void grayscale(GrayscaleMethod method);
    void reduce();
    void error_diffusion(ErrorDiffusionAlgorithm algorithm, bool alternate);
    void ordered(OrderedAlgorithm algorithm);
};


#endif
