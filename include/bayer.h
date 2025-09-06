#ifndef __BAYER_H
#define __BAYER_H

#include <string> // std::string, std::to_string()
#include <vector> // std::vector

class Bayer
{
public:
    Bayer();
    Bayer(int size, int output_levels);
    std::vector<std::vector<int>> get_threshold_matrix();
    void generate_bayer_matrix();

    std::string to_string();

private:
    std::vector<std::vector<int>> threshold_matrix;
    size_t size;
    size_t output_levels;

    std::vector<std::vector<int>> _bayer_matrix(size_t x, size_t y, size_t size, int value, int step, std::vector<std::vector<int>> &matrix);
};

#endif
