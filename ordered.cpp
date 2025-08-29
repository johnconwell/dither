#include "ordered.h"

Ordered::Ordered()
{
    threshold_matrix.resize(0);
}

// fills threshold_matrix with a size x size bayer matrix
void Ordered::bayer_matrix(int size, int output_levels)
{
    threshold_matrix = std::vector<std::vector<int>>(size, std::vector<int>(size, 0));
    _bayer_matrix(0, 0, size, 0, 1, threshold_matrix);

    // normalize threshold matrix
    int height = threshold_matrix.size();
    int width = threshold_matrix[0].size();
    int num_pixels = height * width;

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            threshold_matrix[y][x] *= double(output_levels) / double(num_pixels);
        }
    }
    return;
}

// fills threshold_matrix with a size x size matrix of randomly distributed values between 0-255
void Ordered::white_noise(int size, int output_levels)
{
    threshold_matrix = std::vector<std::vector<int>>(size, std::vector<int>(size, 0));

    for(int y = 0; y < size; y++)
    {
        for(int x = 0; x < size; x++)
        {
            threshold_matrix[y][x] = rand() % output_levels;
        }
    }

    return;
}

// fills threshold_matrix with a size x size matrix of randomly distributed values, following a blue distribution
void Ordered::blue_noise(int size)
{



    return;
}

std::vector<std::vector<int>> Ordered::_bayer_matrix(int x, int y, int size, int value, int step, std::vector<std::vector<int>> &matrix)
{
    if(size == 1)
    {
        matrix[y][x] = value;
        return matrix;
    }
    int half = size / 2;
    _bayer_matrix(x,        y       , half, value + (step * 0), step * 4, matrix);
    _bayer_matrix(x + half, y + half, half, value + (step * 1), step * 4, matrix);
    _bayer_matrix(x + half, y       , half, value + (step * 2), step * 4, matrix);
    _bayer_matrix(x,        y + half, half, value + (step * 3), step * 4, matrix);
    return matrix;
}

// returns the toriodal distance between two cells on a square matrix of size 'size'
int Ordered::_distance_toriodal_squared(int x1, int y1, int x2, int y2, int size)
{
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    double half_size = size / 2;

    if(dx > half_size)
    {
        dx = size - dx;
    }

    if(dy > half_size)
    {
        dy = size - dy;
    }

    return (dx * dx) + (dy * dy);
}

// returns the index of a coordinate pair in a 1d matrix
int _index(int x, int y, int size)
{
    return (y * size) + x;
}

// returns the coordinate pair of an index in a 2d matrix
std::pair<int, int> _coordinate(int index, int size)
{
    return std::pair<int, int>(index % size, index / size);
}

std::string Ordered::to_string()
{
    std::string output = "";
    int size = threshold_matrix.size();

    for(int y = 0; y < size; y++)
    {
        output += "{";
        for(int x = 0; x < size; x++)
        {
            if((size * size) >= 10 && (size * size) < 100 && threshold_matrix[y][x] < 10)
            {
                output += "0";
            }
            else if((size * size) > 100 && threshold_matrix[y][x] < 10)
            {
                output += "00";
            }
            else if((size * size) > 100 && threshold_matrix[y][x] < 100)
            {
                output += "0";
            }
            output += std::to_string(threshold_matrix[y][x]) + ".0 / " + std::to_string(size * size) + ".0";
            if(x < size - 1)
            {
                output += ", ";
            }
        }
        output += "}";
        if(y < size - 1)
        {
            output += ",\n";
        }
    }

    return output;
}