#ifndef __CONVOLVE_H
#define __CONVOLVE_H

#include <cstdlib> // std::exit
#include <filesystem> // std::filesystem::is_empty
#include <fstream> // std::ifstream
#include <iostream> // std::cerr, std::endl
#include <sstream> // std::stringstream
#include <string> // std::string
#include <unordered_map> // std::unordered_map
#include <vector> // std::vector

class Convolve
{
public:
    const std::unordered_map<std::string, std::vector<std::vector<double>>> KERNELS =
    {
        {"RIDGE_4", std::vector<std::vector<double>>({
            {+0.0, -1.0, +0.0},
            {-1.0, +4.0, -1.0},
            {+0.0, -1.0, +0.0}
        })},
        {"RIDGE_8", std::vector<std::vector<double>>({
            {-1.0, -1.0, -1.0},
            {-1.0, +8.0, -1.0},
            {-1.0, -1.0, -1.0}
        })},
        {"SHARPEN_4", std::vector<std::vector<double>>({
            {+0.0, -1.0, +0.0},
            {-1.0, +5.0, -1.0},
            {+0.0, -1.0, +0.0}
        })},
        {"SHARPEN_8", std::vector<std::vector<double>>({
            {-1.0, -1.0, -1.0},
            {-1.0, +9.0, -1.0},
            {-1.0, -1.0, -1.0}
        })},
        {"BOX_BLUR", std::vector<std::vector<double>>({
            {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
            {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
            {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0}
        })},
        {"GAUSSIAN_BLUR", std::vector<std::vector<double>>({
            {0.00296902, 0.0133062, 0.0219382, 0.0133062, 0.00296902},
            {0.0133062, 0.0596343, 0.0983203, 0.0596343, 0.0133062},
            {0.0219382, 0.0983203, 0.162103, 0.0983203, 0.0219382},
            {0.0133062, 0.0596343, 0.0983203, 0.0596343, 0.0133062},
            {0.00296902, 0.0133062, 0.0219382, 0.0133062, 0.00296902}
        })},
        {"UNSHARP_MASK", std::vector<std::vector<double>>({
            {-0.00296902, -0.0133062, -0.0219382, -0.0133062, -0.00296902},
            {-0.0133062, -0.0596343, -0.0983203, -0.0596343, -0.0133062},
            {-0.0219382, -0.0983203, +1.837897, -0.0983203, -0.0219382},
            {-0.0133062, -0.0596343, -0.0983203, -0.0596343, -0.0133062},
            {-0.00296902, -0.0133062, -0.0219382, -0.0133062, -0.00296902}
        })}
    };

    Convolve() = delete;

    Convolve(std::string name)
    {
        if(KERNELS.contains(name))
        {
            this->name = name;
            this->kernel = KERNELS.at(name);
        }
        else
        {
            std::ifstream file;
            file.open(name);

            if(!file.is_open() || std::filesystem::is_empty(name))
            {
                std::cerr << "Error: invalid convolution kernel - " << name << std::endl;
                std::exit(EXIT_FAILURE);
            }

            std::streamoff count = std::count_if(std::istreambuf_iterator<char>{file}, {}, [](char c) { return c == '\n'; });
            file.seekg(-1, std::ios::end);

            if(file.peek() != '\n')
            {
                count++;
            }

            file.seekg(0, std::ios::beg);
            
            std::vector<std::vector<std::string>> tokens;
            tokens.resize(count);
            std::string token;

            for(int index_line = 0; index_line < count; index_line++)
            {
                std::string line;
                std::getline(file, line, '\n');
                std::stringstream ss(line);

                while(std::getline(ss, token, ','))
                {
                    tokens[index_line].push_back(token);
                }

                if(tokens[index_line].size() != tokens[0].size())
                {
                    std::cerr << "Error: kernel width is not consistent across rows: elements in row 1 = " <<  tokens[0].size() << ", elements in row " << (index_line + 1) << " = " << tokens[index_line].size() << std::endl;
                    std::exit(EXIT_FAILURE);
                }
            }

            this->name = name;
            this->kernel.resize(count);

            for(std::size_t y = 0; y < static_cast<std::size_t>(count); y++)
            {
                kernel[y].resize(tokens[y].size());
                
                for(std::size_t x = 0; x < kernel[y].size(); x++)
                {
                    try
                    {
                        this->kernel[y][x] = std::stod(tokens[y][x]);
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << "Error: invalid kernel value found in " << name << " - " << tokens[y][x] << std::endl;
                        std::exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }

    std::string name;
    std::vector<std::vector<double>> kernel;
};


#endif
