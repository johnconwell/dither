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
    size_t get_width();
    size_t get_height();
    size_t load(const char* file_name);
    size_t save(const char* file_name);
    Color get_pixel(unsigned int x, unsigned int y);
    void set_pixel(Color color, unsigned int x, unsigned int y);
    std::vector<std::vector<int>> gradient_map();
    void create_from_threshold_matrix(std::vector<std::vector<int>> threshold_matrix);
};

#endif
