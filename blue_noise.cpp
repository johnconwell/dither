#include "blue_noise.h"

// initialize everything with empty/zero size values
BlueNoise::BlueNoise()
{
    dither_array = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    binary_pattern_initial = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    binary_pattern_prototype = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    minority_pixels = std::vector<std::pair<int, int>>(0, {-1, -1});
    majority_pixels = std::vector<std::pair<int, int>>(0, {-1, -1});
    sigma = 0.0;
    coverage = 0.0;
    kernel = std::vector<std::vector<double>>(0, std::vector<double>(0, 0.0));
    return;
}

// resize all containers and initialize with zeros
BlueNoise::BlueNoise(int width, int height, double sigma, double coverage)
{
    this->dither_array = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->binary_pattern_initial = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->binary_pattern_prototype = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->minority_pixels = std::vector<std::pair<int, int>>(0, {-1, -1});
    this->majority_pixels = std::vector<std::pair<int, int>>(0, {-1, -1});
    this->sigma = sigma;
    this->coverage = coverage;
    this->kernel = std::vector<std::vector<double>>(0, std::vector<double>(0, 0.0));
    return;
}

// generates the values in dither array from 0 to (ones - 1) (number of 1s in initial binary pattern minus 1)
void BlueNoise::generate_dither_array_phase_1()
{
    int height = dither_array.size();
    int width = dither_array[0].size();
    int ones = width * height * coverage;
    int rank = ones - 1;

    load_binary_pattern();

    while(rank >= 0)
    {
        std::pair<int, int> coordinate_tightest_cluster = get_tightest_cluster(binary_pattern_prototype);
        binary_pattern_prototype[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = 0;
        dither_array[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = rank;
        std::vector<std::pair<int, int>>::iterator coordinate_position = std::find(minority_pixels.begin(), minority_pixels.end(), coordinate_tightest_cluster);
        minority_pixels.erase(coordinate_position);
        majority_pixels.push_back(coordinate_tightest_cluster);
        rank--;
    }

    return;
}

// generates the values in dither array from ones to (width * height / 2)
void BlueNoise::generate_dither_array_phase_2()
{
    int height = dither_array.size();
    int width = dither_array[0].size();
    int num_pixels_half = width * height / 2;
    int ones = width * height * coverage;
    int rank = ones;

    load_binary_pattern();

    while(rank < num_pixels_half)
    {
        std::pair<int, int> coordinate_largest_void = get_largest_void(binary_pattern_prototype);
        binary_pattern_prototype[coordinate_largest_void.second][coordinate_largest_void.first] = 1;
        dither_array[coordinate_largest_void.second][coordinate_largest_void.first] = rank;
        std::vector<std::pair<int, int>>::iterator coordinate_position = std::find(majority_pixels.begin(), majority_pixels.end(), coordinate_largest_void);
        majority_pixels.erase(coordinate_position);
        minority_pixels.push_back(coordinate_largest_void);
        rank++;
    }

    return;
}

// generates the values in dither array from (width * height / 2) to number of pixels in dither_array
void BlueNoise::generate_dither_array_phase_3()
{
    int height = dither_array.size();
    int width = dither_array[0].size();
    int num_pixels = width * height;
    int num_pixels_half = num_pixels / 2;
    int rank = num_pixels_half + 1;

    // TODO: make functions to regenerate minority/majority pixel arrays
    // these are not wokring for some reason
    std::cout << "proto pre invert:" << std::endl;
    std::cout << to_string_binary_pattern_prototype() << std::endl;
    invert_binary_pattern();
    std::cout << "proto POST invert:" << std::endl;
    std::cout << to_string_binary_pattern_prototype() << std::endl;

    while(rank < num_pixels)
    {
        std::pair<int, int> coordinate_tightest_cluster = get_tightest_cluster(binary_pattern_prototype);
        std::cout << rank << " " << num_pixels << std::endl;
        // std::cout << coordinate_tightest_cluster.first << " " << coordinate_tightest_cluster.second << std::endl;
        binary_pattern_prototype[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = 1;
        dither_array[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = rank;
        std::vector<std::pair<int, int>>::iterator coordinate_position = std::find(minority_pixels.begin(), minority_pixels.end(), coordinate_tightest_cluster);
        minority_pixels.erase(coordinate_position);
        majority_pixels.push_back(coordinate_tightest_cluster);
        rank++;
    }

    return;
}

// generates the initial binary pattern by randomly placing (width * height * coverage) ones, then evenly spacing them using void and cluster algorithm
// also fills and updates minority/majority pixel arrays
void BlueNoise::generate_initial_binary_pattern()
{
    int height = binary_pattern_initial.size();
    int width = binary_pattern_initial[0].size();
    int num_pixels = height * width;
    int num_minority_pixels = num_pixels * coverage;
    std::vector<std::pair<int, int>> remaining_coordinates = std::vector<std::pair<int, int>>(num_pixels, {-1, -1});
    std::vector<std::pair<int, int>>::iterator coordinate_position;

    // fill remaining_coordinates with all coordinates in the binary pattern matrix
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            int index_remaining_coordinates = index(x, y, width, height);
            remaining_coordinates[index(x, y, width, height)].first = x;
            remaining_coordinates[index(x, y, width, height)].second = y;
        }
    }

    // shuffle remaining coordinates array
    for(int index_remaining_coordinates = num_pixels - 1; index_remaining_coordinates >= 0; index_remaining_coordinates--)
    {
        int index_random = rand() % (index_remaining_coordinates + 1);
        std::pair<int, int> temp = remaining_coordinates[index_remaining_coordinates];
        remaining_coordinates[index_remaining_coordinates] = remaining_coordinates[index_random];
        remaining_coordinates[index_random] = temp;
    }

    // fill binary pattern with num_minority_pixels minority pixels (ones)
    // also fill minority_pixels and majority_pixels arrays
    for(int index_remaining_coordinates = 0; index_remaining_coordinates < num_pixels; index_remaining_coordinates++)
    {
        std::pair<int, int> coordinate = remaining_coordinates[index_remaining_coordinates];
        if(index_remaining_coordinates < num_minority_pixels)
        {
            binary_pattern_initial[coordinate.second][coordinate.first] = 1;
            minority_pixels.push_back(coordinate);
        }
        else
        {
            // binary pattern pixels are already initialized to 0, so no need to do that now
            majority_pixels.push_back(coordinate);
        }
    }
    
    int count = 0;
    while(count < 100)
    {
        // find the tightest cluster and remove it
        std::pair<int, int> coordinate_tightest_cluster = get_tightest_cluster(binary_pattern_initial);
        binary_pattern_initial[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = 0;
        coordinate_position = std::find(minority_pixels.begin(), minority_pixels.end(), coordinate_tightest_cluster);
        minority_pixels.erase(coordinate_position);
        majority_pixels.push_back(coordinate_tightest_cluster);

        // find the largest void and fill it
        std::pair<int, int> coordinate_largest_void = get_largest_void(binary_pattern_initial);
        binary_pattern_initial[coordinate_largest_void.second][coordinate_largest_void.first] = 1;
        coordinate_position = std::find(majority_pixels.begin(), majority_pixels.end(), coordinate_largest_void);
        majority_pixels.erase(coordinate_position);
        minority_pixels.push_back(coordinate_largest_void);

        // exit condition when removing the tightest cluster creates the largest void
        if((coordinate_tightest_cluster.first == coordinate_largest_void.first) && (coordinate_tightest_cluster.second == coordinate_largest_void.second))
        {
            break;
        }

        count += 1;
    }

    return;
}

// returns the coordinates of the tightest cluster in the binary_pattern using gausian filter
std::pair<int, int> BlueNoise::get_tightest_cluster(std::vector<std::vector<int>> binary_pattern)
{
    std::pair<int, int> coordinate_tightest_cluster = std::pair<int, int>(-1, -1);
    double value_tightest_cluster = 0.0;

    for(int index_minority_pixels = 0; index_minority_pixels < minority_pixels.size(); index_minority_pixels++)
    {
        std::pair<int, int> coordinate = minority_pixels[index_minority_pixels];
        double value = gaussian_value(binary_pattern, coordinate.first, coordinate.second);

        if(value > value_tightest_cluster)
        {
            value_tightest_cluster = value;
            coordinate_tightest_cluster = coordinate;
        }
    }

    return coordinate_tightest_cluster;
}

// returns the coordinates of the largest void in the binary_pattern using gausian filter
std::pair<int, int> BlueNoise::get_largest_void(std::vector<std::vector<int>> binary_pattern)
{
    std::pair<int, int> coordinate_largest_void = std::pair<int, int>(-1, -1);
    double value_largest_void = DBL_MAX;
    
    for(int index_majority_pixels = 0; index_majority_pixels < majority_pixels.size(); index_majority_pixels++)
    {
        std::pair<int, int> coordinate = majority_pixels[index_majority_pixels];
        double value = gaussian_value(binary_pattern, coordinate.first, coordinate.second);

        if(value < value_largest_void)
        {
            value_largest_void = value;
            coordinate_largest_void = coordinate;
        }
    }

    return coordinate_largest_void;
}

// applies gausian filter to the pixel at specified coordinates and returns the result
double BlueNoise::gaussian_value(std::vector<std::vector<int>> binary_pattern, int x, int y)
{
    int height = binary_pattern.size();
    int width = binary_pattern[0].size();
    double value = 0.0;
    double two_sigma_squared = 2 * sigma * sigma;

    // calculate the contribution of each pixel in the binary pattern
    for(int j = 0; j < height; j++)
    {
        for(int i = 0; i < width; i++)
        {
            // only pixels of value 1 contribute to the energy
            if(binary_pattern[j][i] == 1)
            {
                std::pair<int, int> p1 = {x, y};
                std::pair<int, int> p2 = {i, j};
                double distance_squared = distance_torroidal_squared(p1, p2, width, height);
                value += exp(-distance_squared / (two_sigma_squared));
            }
        }
    }

    return value;
}

// loads binary_pattern_prototype with the contents of binary_pattern_initial
void BlueNoise::load_binary_pattern()
{
    int height = binary_pattern_initial.size();
    int width = binary_pattern_initial[0].size();

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            binary_pattern_prototype[y][x] = binary_pattern_initial[y][x];
        }
    }

    return;
}

// inverts binary_pattern_prototype (switches all 0s to 1s and vice versa)
// also switches minority and majority pixels
void BlueNoise::invert_binary_pattern()
{
    int height = binary_pattern_prototype.size();
    int width = binary_pattern_prototype[0].size();

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            std::pair<int, int> coordinate = std::pair<int, int>(x, y);
            
            if(binary_pattern_prototype[y][x] == 1)
            {
                binary_pattern_prototype[y][x] = 0;
                std::vector<std::pair<int, int>>::iterator coordinate_position = std::find(minority_pixels.begin(), minority_pixels.end(), coordinate);
                if(coordinate_position == minority_pixels.end())
                {
                    std::cout << "can find in minority: " << x << " " << y << std::endl;
                }
                minority_pixels.erase(coordinate_position);
                majority_pixels.push_back(coordinate);
            }
            else
            {
                binary_pattern_prototype[y][x] = 1;
                std::vector<std::pair<int, int>>::iterator coordinate_position = std::find(majority_pixels.begin(), majority_pixels.end(), coordinate);
                if(coordinate_position == majority_pixels.end())
                {
                    std::cout << "can find in majority: " << x << " " << y << std::endl;
                }
                majority_pixels.erase(coordinate_position);
                minority_pixels.push_back(coordinate);
            }
        }
    }

    return;
}

// returns the distance squared between two points in torroidal space
double BlueNoise::distance_torroidal_squared(std::pair<int, int> p1, std::pair<int, int> p2, int width, int height)
{
    double dx = std::abs(double(p2.first) - double(p1.first));
    double dy = std::abs(double(p2.second) - double(p1.second));
    double half_width = double(width) / 2.0;
    double half_height = double(height) / 2.0;
    
    if(dx > half_width)
    {
        dx = half_height - dx;
    }
    if(dy > half_height)
    {
        dy = half_height - dy;
    }

    return (dx * dx) + (dy * dy);
}

int BlueNoise::index(int x, int y, int width, int height)
{
    return (y * height) + x;
}

void BlueNoise::kernel_gaussian(double sigma, int width, int height)
{
    // resize the kernel and fill it with zeros
    kernel.resize(height);
    for(int y = 0; y < height; y++)
    {
        kernel[y].assign(width, 0.0);
    }

    double sum = 0.0;
    int half_width = width / 2;
    int half_height = height / 2;
    double two_sigma_squared = 2.0 * sigma * sigma;

    // fill the kernel based on the gaussian function
    for(int y = -half_height; y < half_height + 1; y++)
    {
        for(int x = -half_width; x < half_width + 1; x++)
        {
            kernel[y + half_height][x + half_width] = (exp(-1 * (x * x + y * y) / two_sigma_squared)) / (M_PI * two_sigma_squared);
            sum += kernel[y + half_height][x + half_width];
        }
    }

    // normalize the kernel
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            kernel[y][x] /= sum;
        }
    }

    return;
}

std::string BlueNoise::to_string_dither_array()
{
    int height = dither_array.size();
    int width = dither_array[0].size();
    std::string output = "";

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            output += std::to_string(dither_array[y][x]) + "\t";
        }

        output += "\n";
    }

    return output;
}

std::string BlueNoise::to_string_binary_pattern_initial()
{
    int height = binary_pattern_initial.size();
    int width = binary_pattern_initial[0].size();
    std::string output = "";

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            output += std::to_string(binary_pattern_initial[y][x]) + " ";
        }
        
        output += "\n";
    }

    return output;
}

std::string BlueNoise::to_string_binary_pattern_prototype()
{
    int height = binary_pattern_prototype.size();
    int width = binary_pattern_prototype[0].size();
    std::string output = "";

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            output += std::to_string(binary_pattern_prototype[y][x]) + " ";
        }
        
        output += "\n";
    }

    return output;
}
