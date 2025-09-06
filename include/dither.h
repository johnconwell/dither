#ifndef __DITHER_H
#define __DITHER_H

#include "error_diffusion.h" // for error diffusion constants
#include "grayscale.h" // channel_value()
#include "image.h" // data structure for colors
#include "lodepng.h" // for loading input pngs and saving to output pngs
#include "palette.h" // datastructure for colors
#include <vector> // std::vector

class Dither
{
public:
    Dither();
    void set_palette(Palette palette);
    size_t load(const char* file_name);
    size_t save(const char* file_name);
    void grayscale(GrayscaleMethod method);
    void reduce();
    void error_diffusion(ErrorDiffusionAlgorithm algorithm, bool alternate);
    void ordered(std::vector<std::vector<int>> threshold_matrix);

private:
    Image image;
    Palette palette;

    void error_diffusion_standard(ErrorDiffusionAlgorithm algorithm);
    void error_diffusion_alternate(ErrorDiffusionAlgorithm algorithm);
};

#endif
