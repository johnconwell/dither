#ifndef __DITHER_H
#define __DITHER_H

#include "error.h"
#include "grayscale.h"
#include "image.h"
#include "lodepng.h"
#include "palette.h"
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
    unsigned int load(const char* file_name);
    unsigned int save(const char* file_name);
    void grayscale(GrayscaleMethod method);
    void reduce();
    void error_diffusion(ErrorDiffusionAlgorithm algorithm, bool alternate);
    void error_diffusion_grayscale_standard(ErrorDiffusionAlgorithm algorithm);
    
};


#endif
