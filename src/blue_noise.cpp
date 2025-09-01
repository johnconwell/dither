#include "blue_noise.h"

EnergyLUT::EnergyLUT()
{
    height = 0;
    width = 0;
    LUT = std::vector<std::vector<double>>(0, std::vector<double>(0, 0.0));
    value_lowest_energy = DBL_MAX;
    value_highest_energy = 0.0;
    coordinate_lowest_energy = std::pair<int, int>(-1, -1);
    coordinate_highest_energy = std::pair<int, int>(-1, -1);
}

EnergyLUT::EnergyLUT(size_t width, size_t height)
{
    this->height = height;
    this->width = width;
    LUT = std::vector<std::vector<double>>(height, std::vector<double>(width, 0.0));
    value_lowest_energy = DBL_MAX;
    value_highest_energy = DBL_MIN;
    coordinate_lowest_energy = std::pair<int, int>(-1, -1);
    coordinate_highest_energy = std::pair<int, int>(-1, -1);
}

// populates the energy LUT from a new binary pattern
void EnergyLUT::create(std::vector<std::vector<int>> binary_pattern, double sigma)
{
    value_lowest_energy = DBL_MAX;
    value_highest_energy = DBL_MIN;
    double half_width = static_cast<double>(width) / 2.0;
    double half_height = static_cast<double>(height) / 2.0;
    double two_sigma_squared = 2 * sigma * sigma;

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            // reset LUT value before calculating energy
            LUT[y][x] = 0.0;

            // calculate the contribution of each pixel in the binary pattern
            for(size_t j = 0; j < height; j++)
            {
                double dy = std::abs(static_cast<double>(y) - static_cast<double>(j));

                if(dy > half_height)
                {
                    dy = height - dy;
                }

                for(size_t i = 0; i < width; i++)
                {
                    // only pixels of value 1 contribute to the energy
                    if(binary_pattern[j][i] == 1)
                    {
                        double dx = std::abs(static_cast<double>(x) - static_cast<double>(i));
                
                        if(dx > half_width)
                        {
                            dx = width - dx;
                        }

                        LUT[y][x] += exp(-1 * ((dx * dx) + (dy * dy)) / two_sigma_squared);
                    }
                }
            }

            // only 0s (majority pixels) in the binary pattern can be considered the largest void
            if(binary_pattern[y][x] == 0 && LUT[y][x] < value_lowest_energy)
            {
                value_lowest_energy = LUT[y][x];
                coordinate_lowest_energy = std::pair<int, int>(x, y);
            }

            // only 1s (minority pixels) in the binary pattern can be considered the largest cluster
            if(binary_pattern[y][x] == 1 && LUT[y][x] > value_highest_energy)
            {
                value_highest_energy = LUT[y][x];
                coordinate_highest_energy = std::pair<int, int>(x, y);
            }
        }
    }
}

// updates each element of the LUT to account for a change in the binary pattern
void EnergyLUT::update(std::vector<std::vector<int>> binary_pattern, size_t x, size_t y, double sigma)
{
    double half_width = static_cast<double>(width) / 2.0;
    double half_height = static_cast<double>(height) / 2.0;
    double two_sigma_squared = 2 * sigma * sigma;

    value_lowest_energy = DBL_MAX;
    value_highest_energy = 0.0;

    // calculate the contribution of each pixel in the binary pattern
    for(size_t j = 0; j < height; j++)
    {
        double dy = std::abs(static_cast<double>(y) - static_cast<double>(j));

        if(dy > half_height)
        {
            dy = height - dy;
        }

        for(size_t i = 0; i < width; i++)
        {
            double dx = std::abs(static_cast<double>(x) - static_cast<double>(i));
    
            if(dx > half_width)
            {
                dx = width - dx;
            }

            double gaussian_value = exp(-1 * ((dx * dx) + (dy * dy)) / two_sigma_squared);

            // if the newly updated pixel is a 1, then add to the other pixels
            if(binary_pattern[y][x] == 1)
            {
                LUT[j][i] += gaussian_value;
            }
            // if the newly updated pixel is a 0, then subtract from the other pixels
            else
            {
                LUT[j][i] -= gaussian_value;
            }

            // only 0s (majority pixels) in the binary pattern can be considered the largest void
            if(binary_pattern[j][i] == 0 && LUT[j][i] < value_lowest_energy)
            {
                value_lowest_energy = LUT[j][i];
                coordinate_lowest_energy = std::pair<int, int>(i, j);
            }

            // only 1s (minority pixels) in the binary pattern can be considered the largest cluster
            if(binary_pattern[j][i] == 1 && LUT[j][i] > value_highest_energy)
            {
                value_highest_energy = LUT[j][i];
                coordinate_highest_energy = std::pair<int, int>(i, j);
            }
        }
    }

    return;
}

std::string EnergyLUT::to_string()
{
    std::string output = "";

    output += "Coord Highest: (" + std::to_string(coordinate_highest_energy.first) + ", " + std::to_string(coordinate_highest_energy.second) + ")\tValue Highest: " + std::to_string(value_highest_energy) + "\n";
    output += "Coord Lowest:  (" + std::to_string(coordinate_lowest_energy.first) + ", " + std::to_string(coordinate_lowest_energy.second) + ")\tValue Lowest:  " + std::to_string(value_lowest_energy) + "\n";

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(LUT[y][x]) + "\t";
        }
        
        output += "\n";
    }

    return output;
}

// initialize everything with empty/zero size values
BlueNoise::BlueNoise()
{
    dither_array = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    binary_pattern_initial = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    binary_pattern_prototype = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    energy_lut = EnergyLUT();
    width = 0;
    height = 0;
    sigma = 0.0;
    coverage = 0.0;
    output_levels = 0;
    return;
}

// resize all containers and initialize with zeros
BlueNoise::BlueNoise(size_t width, size_t height, double sigma, double coverage, size_t output_levels)
{
    this->dither_array = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->binary_pattern_initial = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->binary_pattern_prototype = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->energy_lut = EnergyLUT(width, height);
    this->width = width;
    this->height = height;
    this->sigma = sigma;
    this->coverage = coverage;
    this->output_levels = output_levels;

    // must have at least one pixel set high
    if(static_cast<double>(this->width) * static_cast<double>(this->height) * this->coverage < 1.0)
    {
        this->coverage = 1.0 / (static_cast<double>(this->width) * static_cast<double>(this->height));
    }
    return;
}

// returns the dither array
std::vector<std::vector<int>> BlueNoise::get_threshold_matrix()
{
    return dither_array;
}

// performs the full blue noise algorithm
void BlueNoise::generate_blue_noise()
{
    generate_initial_binary_pattern();
    generate_dither_array_phase_1();
    generate_dither_array_phase_2();
    generate_dither_array_phase_3();
    normalize_dither_array();
}

// generates the initial binary pattern by randomly placing (width * height * coverage) ones, then evenly spacing them using void and cluster algorithm
void BlueNoise::generate_initial_binary_pattern()
{
    size_t num_pixels = height * width;
    size_t num_minority_pixels = num_pixels * coverage;
    std::vector<std::pair<int, int>> remaining_coordinates = std::vector<std::pair<int, int>>(num_pixels, {-1, -1});
    std::random_device rd;
    std::mt19937 mt(rd());

    // fill remaining_coordinates with all coordinates in the binary pattern matrix
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            // ((y * height) + x) maps 2d array coords to a 1d array
            size_t index_remaining_coordinates = (y * height) + x;
            remaining_coordinates[index_remaining_coordinates].first = x;
            remaining_coordinates[index_remaining_coordinates].second = y;
        }
    }

    // shuffle remaining coordinates array
    for(int index_remaining_coordinates = num_pixels - 1; index_remaining_coordinates >= 0; index_remaining_coordinates--)
    {
        std::uniform_int_distribution<> dis(0, index_remaining_coordinates);
        int index_random = dis(mt);
        std::pair<int, int> temp = remaining_coordinates[index_remaining_coordinates];
        remaining_coordinates[index_remaining_coordinates] = remaining_coordinates[index_random];
        remaining_coordinates[index_random] = temp;
    }

    // fill binary pattern with num_minority_pixels ones
    for(size_t index_remaining_coordinates = 0; index_remaining_coordinates < num_pixels; index_remaining_coordinates++)
    {
        std::pair<int, int> coordinate = remaining_coordinates[index_remaining_coordinates];
        if(index_remaining_coordinates < num_minority_pixels)
        {
            binary_pattern_initial[coordinate.second][coordinate.first] = 1;
        }
    }

    energy_lut.create(binary_pattern_initial, sigma);

    // limiting iterations ensures that this will not get stuck forever if there is a bug
    int count = 0;
    while(count < 100)
    {
        // find the tightest cluster and remove it
        std::pair<int, int> coordinate_tightest_cluster = energy_lut.coordinate_highest_energy;
        binary_pattern_initial[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = 0;
        energy_lut.update(binary_pattern_initial, coordinate_tightest_cluster.first, coordinate_tightest_cluster.second, sigma);

        // find the largest void and fill it
        std::pair<int, int> coordinate_largest_void = energy_lut.coordinate_lowest_energy;
        binary_pattern_initial[coordinate_largest_void.second][coordinate_largest_void.first] = 1;
        energy_lut.update(binary_pattern_initial, coordinate_largest_void.first, coordinate_largest_void.second, sigma);

        // exit condition when removing the tightest cluster creates the largest void
        if((coordinate_tightest_cluster.first == coordinate_largest_void.first) && (coordinate_tightest_cluster.second == coordinate_largest_void.second))
        {
            break;
        }

        count += 1;
    }

    return;
}

// generates the values in dither array from 0 to (ones - 1) (number of 1s in initial binary pattern minus 1)
void BlueNoise::generate_dither_array_phase_1()
{
    int ones = width * height * coverage;
    int rank = ones - 1;

    binary_pattern_copy(binary_pattern_initial, binary_pattern_prototype);
    energy_lut.create(binary_pattern_prototype, sigma);

    while(rank >= 0)
    {
        std::pair<int, int> coordinate_tightest_cluster = energy_lut.coordinate_highest_energy;
        binary_pattern_prototype[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = 0;
        energy_lut.update(binary_pattern_prototype, coordinate_tightest_cluster.first, coordinate_tightest_cluster.second, sigma);
        dither_array[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = rank;
        rank--;
    }

    return;
}

// generates the values in dither array from ones to (width * height / 2)
void BlueNoise::generate_dither_array_phase_2()
{
    int num_pixels_half = width * height / 2;
    int ones = width * height * coverage;
    int rank = ones;

    binary_pattern_copy(binary_pattern_initial, binary_pattern_prototype);
    energy_lut.create(binary_pattern_prototype, sigma);

    while(rank < num_pixels_half)
    {
        std::pair<int, int> coordinate_largest_void = energy_lut.coordinate_lowest_energy;
        binary_pattern_prototype[coordinate_largest_void.second][coordinate_largest_void.first] = 1;
        energy_lut.update(binary_pattern_prototype, coordinate_largest_void.first, coordinate_largest_void.second, sigma);
        dither_array[coordinate_largest_void.second][coordinate_largest_void.first] = rank;
        rank++;
    }

    return;
}

// generates the values in dither array from (width * height / 2) to number of pixels in dither_array
void BlueNoise::generate_dither_array_phase_3()
{
    int num_pixels = width * height;
    int num_pixels_half = num_pixels / 2;
    int rank = num_pixels_half;

    binary_pattern_invert(binary_pattern_prototype);
    energy_lut.create(binary_pattern_prototype, sigma);

    while(rank < num_pixels)
    {
        std::pair<int, int> coordinate_tightest_cluster = energy_lut.coordinate_highest_energy;
        binary_pattern_prototype[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = 0;
        energy_lut.update(binary_pattern_prototype, coordinate_tightest_cluster.first, coordinate_tightest_cluster.second, sigma);
        dither_array[coordinate_tightest_cluster.second][coordinate_tightest_cluster.first] = rank;
        rank++;
    }

    return;
}


void BlueNoise::normalize_dither_array()
{
    int num_pixels = height * width;

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            dither_array[y][x] *= static_cast<double>(output_levels) / static_cast<double>(num_pixels);
        }
    }

    return;
}

// copies binary_pattern_source into binary_pattern_destination
void BlueNoise::binary_pattern_copy(std::vector<std::vector<int>> &binary_pattern_source, std::vector<std::vector<int>> &binary_pattern_destination)
{
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            binary_pattern_destination[y][x] = binary_pattern_source[y][x];
        }
    }

    return;
}

// inverts specified binary_pattern (switches all 0s to 1s and vice versa)
void BlueNoise::binary_pattern_invert(std::vector<std::vector<int>> &binary_pattern)
{
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            binary_pattern[y][x] = binary_pattern[y][x] ? 0 : 1;
        }
    }

    return;
}

std::string BlueNoise::to_string_dither_array()
{
    std::string output = "";

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(dither_array[y][x]) + "\t";
        }

        output += "\n";
    }

    return output;
}

std::string BlueNoise::to_string_binary_pattern_initial()
{
    std::string output = "";

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(binary_pattern_initial[y][x]) + " ";
        }
        
        output += "\n";
    }

    return output;
}

std::string BlueNoise::to_string_binary_pattern_prototype()
{
    std::string output = "";

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(binary_pattern_prototype[y][x]) + " ";
        }
        
        output += "\n";
    }

    return output;
}
