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
    void clear();
    unsigned int get_width();
    unsigned int get_height();
    unsigned int load(const char* file_name);
    unsigned int save(const char* file_name);
    Color get_pixel(int x, int y);
    void set_pixel(Color color, int x, int y);
    std::vector<std::vector<int>> gradient_map();
    void create_from_threshold_matrix(std::vector<std::vector<int>> threshold_matrix);
};

#endif
