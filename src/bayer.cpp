#include "bayer.h"

Bayer::Bayer()
{
    threshold_matrix = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    size = 0;
    output_levels = 0;
    return;
}

Bayer::Bayer(int size, int output_levels)
{
    this->threshold_matrix = std::vector<std::vector<int>>(size, std::vector<int>(size, 0));
    this->size = size;
    this->output_levels = output_levels;
    return;
}

std::vector<std::vector<int>> Bayer::get_threshold_matrix()
{
    return threshold_matrix;
}

// generates a size x size bayer matrix
void Bayer::generate_bayer_matrix()
{
    _bayer_matrix(0, 0, size, 0, 1, threshold_matrix);

    // normalize threshold matrix once recursion has completed
    int num_pixels = size * size;

    for(size_t y = 0; y < size; y++)
    {
        for(size_t x = 0; x < size; x++)
        {
            threshold_matrix[y][x] *= double(output_levels) / double(num_pixels);
        }
    }

    return;
}

// recursively populate a bayer matrix
std::vector<std::vector<int>> Bayer::_bayer_matrix(size_t x, size_t y, size_t size, int value, int step, std::vector<std::vector<int>> &matrix)
{
    if(size == 1)
    {
        matrix[y][x] = value;
        return matrix;
    }

    size_t half = size / 2;

    _bayer_matrix(x,        y       , half, value + (step * 0), step * 4, matrix);
    _bayer_matrix(x + half, y + half, half, value + (step * 1), step * 4, matrix);
    _bayer_matrix(x + half, y       , half, value + (step * 2), step * 4, matrix);
    _bayer_matrix(x,        y + half, half, value + (step * 3), step * 4, matrix);

    return matrix;
}

std::string Bayer::to_string()
{
    std::string output = "";

    for(size_t y = 0; y < size; y++)
    {
        for(size_t x = 0; x < size; x++)
        {
            output += std::to_string(threshold_matrix[y][x]) + " ";
        }
        
        output += "\n";
    }

    return output;
}
