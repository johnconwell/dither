#include "ordered.h"

Ordered::Ordered()
{
    threshold_matrix.resize(0);
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