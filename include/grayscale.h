#ifndef __GRAYSCALE_H
#define __GRAYSCALE_H

#include "color.h"
#include <cstdlib> // std::exit
#include <fstream> // std::ifstream
#include <iostream> // std::cerr, std::endl
#include <sstream> // std::stringstream
#include <string> // std::string
#include <unordered_map> // std::unordered_map
#include <vector> // std::vector

class Grayscale
{
public:
    const std::unordered_map<std::string, std::vector<double>> METHODS =
    {
        {"STANDARD", {0.3334, 0.3333, 0.3333}},
        {"BT709", {0.2126, 0.7152, 0.0722}},
        {"CHANNEL_R", {1.0, 0.0, 0.0}},
        {"CHANNEL_G", {0.0, 1.0, 0.0}},
        {"CHANNEL_B", {0.0, 0.0, 1.0}},
        {"CHANNEL_RG", {0.5, 0.5, 0.0}},
        {"CHANNEL_RB", {0.5, 0.0, 0.5}},
        {"CHANNEL_GB", {0.0, 0.5, 0.5}}
    };

    Grayscale() = delete;

    Grayscale(std::string name)
    {
        if(METHODS.contains(name))
        {
            this->name = name;
            this->weights = METHODS.at(name);
        }
        else
        {
            std::ifstream file;
            file.open(name);

            if(!file.is_open())
            {
                std::cerr << "Error: Invalid greyscale method - " << name << std::endl;
                std::exit(EXIT_FAILURE);
            }
            
            std::string line;
            std::vector<std::string> tokens;
            std::string token;

            std::getline(file, line);
            std::stringstream ss(line);

            while(std::getline(ss, token, ','))
            {
                tokens.push_back(token);
            }

            if(tokens.size() != (Color::NUM_BYTES - 1))
            {
                std::cerr << "Error: File " << name << "should contain " << (Color::NUM_BYTES - 1) << " elements, but actually contains " << tokens.size() << std::endl;
                std::exit(EXIT_FAILURE);
            }

            this->name = name;
            this->weights.resize((Color::NUM_BYTES - 1));

            for(std::size_t index_channel = 0; index_channel < Color::NUM_BYTES - 1; index_channel++)
            {
                try
                {
                    this->weights[index_channel] = std::stod(tokens[index_channel]);
                }
                catch(const std::exception& e)
                {
                    std::cerr << "Error: invalid grayscale weight found in " << name << " - " << tokens[index_channel] << std::endl;
                    std::exit(EXIT_FAILURE);
                }
            }
        }
    }

    std::string name;
    std::vector<double> weights;
};

#endif
