#ifndef __IMAGE_H
#define __IMAGE_H

#include "color.h"
#include "lodepng.h"
#include <vector>

class Image
{
private:
    std::vector<unsigned char> pixels;
    unsigned int width;
    unsigned int height;

public:
    Image();
    unsigned int get_width();
    unsigned int get_height();
    unsigned int load(const char* file_name);
    unsigned int save(const char* file_name);
    Color get_pixel(int x, int y);
    void set_pixel(Color color, int x, int y);
};

#endif
