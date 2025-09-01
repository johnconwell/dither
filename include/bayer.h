#ifndef __BAYER_H
#define __BAYER_H

#include <string>
#include <vector>

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
    int size;
    int output_levels;

    std::vector<std::vector<int>> _bayer_matrix(int x, int y, int size, int value, int step, std::vector<std::vector<int>> &matrix);
};

#endif
