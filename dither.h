#ifndef __DITHER_H
#define __DITHER_H

#include "grayscale.h"
#include "image.h"
#include "lodepng.h"
#include <iostream>
#include <vector>




class Dither
{
private:
    Image image;

public:
    Dither();
    unsigned int load(const char* file_name);
    unsigned int save(const char* file_name);
    void grayscale(GrayscaleMethod method);
    void reduce();
};


#endif
