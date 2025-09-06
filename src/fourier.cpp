#include "fourier.h"

Fourier2D::Fourier2D()
{
    dataset = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    height = 0;
    width = 0;
    transform = std::vector<std::vector<int>>(0, std::vector<int>(0, 0));
    remove_dc_offset = false;
    center_output = false;
    return;
}

Fourier2D::Fourier2D(std::vector<std::vector<int>> dataset, bool remove_dc_offset, bool center_output)
{
    this->dataset = dataset;
    this->height = dataset.size();
    this->width = dataset[0].size();
    this->transform = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));
    this->remove_dc_offset = remove_dc_offset;
    this->center_output = center_output;
    return;
}

// returns the dataset
std::vector<std::vector<int>> Fourier2D::get_dataset()
{
    return dataset;
}

// returns the transform
std::vector<std::vector<int>> Fourier2D::get_transform()
{
    return transform;
}

// computes the dft of the dataset and stores the result in transform
void Fourier2D::dft()
{
    size_t size = width * height;

    fftw_complex *in = fftw_alloc_complex(size);
    fftw_complex *out = fftw_alloc_complex(size);
    fftw_plan plan = fftw_plan_dft_2d(height, width, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // fill the in array with data from dataset
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            in[y * height + x][0] = static_cast<double>(dataset[y][x]);
            in[y * height + x][1] = 0.0;
        }
    }

    // remove DC offset by subtracting the mean of the data
    if(remove_dc_offset)
    {
        int mean = 0;

        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                mean += in[y * height + x][0];
            }
        }

        mean /= size;

        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                in[y * height + x][0] -= mean;
            }
        }
    }

    // center output
    if(center_output)
    {
        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                in[y * height + x][0] *= pow(-1, y + x);
            }
        }
    }

    fftw_execute(plan);

    // fill output vector with the results of the transform
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            transform[y][x] = static_cast<int>(magnitude(out[y * height + x]));
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return;
}

// computes the inverse dft of the transform and stores the result in dataset
void Fourier2D::idft()
{
    size_t size = width * height;

    fftw_complex *in = fftw_alloc_complex(size);
    fftw_complex *out = fftw_alloc_complex(size);
    fftw_plan plan = fftw_plan_dft_2d(height, width, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // fill the in array with data from dataset
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            in[y * height + x][0] = static_cast<double>(transform[y][x]);
            in[y * height + x][1] = 0.0;
        }
    }

    // remove DC offset by subtracting the mean of the data
    if(remove_dc_offset)
    {
        int mean = 0;

        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                mean += in[y * height + x][0];
            }
        }

        mean /= size;

        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                in[y * height + x][0] -= mean;
            }
        }
    }

    // center output
    if(center_output)
    {
        for(size_t y = 0; y < height; y++)
        {
            for(size_t x = 0; x < width; x++)
            {
                in[y * height + x][0] *= pow(-1, y + x);
            }
        }
    }

    fftw_execute(plan);

    // fill output vector with the results of the transform
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            dataset[y][x] = static_cast<int>(magnitude(out[y * height + x]));
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return;
}

// normalizes the dataset (maps values to [0, output_levels - 1])
void Fourier2D::normalize_dataset(size_t output_levels)
{
    normalize(dataset, output_levels);
    return;
}

// normalize the transform (maps values to [0, output_levels - 1])
void Fourier2D::normalize_transform(size_t output_levels)
{
    normalize(transform, output_levels);
    return;
}

std::string Fourier2D::to_string()
{
    std::string output = "";

    output += "Dataset:\n";
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(dataset[y][x]) + " ";
        }
        
        output += "\n";
    }

    output += "Transform:\n";
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            output += std::to_string(transform[y][x]) + " ";
        }
        
        output += "\n";
    }

    return output;
}

// returns the magnitude of a complex value
double Fourier2D::magnitude(fftw_complex& value)
{
    return sqrt(value[0] * value[0] + value[1] * value[1]);
}

// normalizes the array (maps values to [0, output_levels - 1])
void Fourier2D::normalize(std::vector<std::vector<int>> &array, size_t output_levels)
{
    int array_min = INT_MAX;
    int array_max = INT_MIN;

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            array_min = std::min(array_min, array[y][x]);
            array_max = std::max(array_max, array[y][x]);
        }
    }

    int array_range = array_max - array_min;

    // normalize to output levels
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            array[y][x] = (array[y][x] - array_min) * (output_levels - 1) / array_range;
        }
    }

    return;
}
