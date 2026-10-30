#include "csv.h"
#include "error.h"
#include <charconv> // std::from_chars
#include <fstream> // std::ifstream
#include <sstream> // std::stringstream

namespace
{
    std::string_view trim(std::string_view text)
    {
        const auto first = text.find_first_not_of(" \t");

        if(first == std::string_view::npos)
        {
            return {};
        }

        const auto last = text.find_last_not_of(" \t");
        return text.substr(first, last - first + 1);
    }

    // the whole token must be a number; std::from_chars doesn't accept a leading '+', so skip it
    bool parse_number(std::string_view token, double& value)
    {
        token = trim(token);

        if(token.size() > 1 && token.front() == '+' && token[1] != '-' && token[1] != '+')
        {
            token.remove_prefix(1);
        }

        const char* last = token.data() + token.size();
        const auto [ptr, ec] = std::from_chars(token.data(), last, value);
        return !token.empty() && ec == std::errc() && ptr == last;
    }
}

csv::Matrix csv::read_matrix(const std::string& path, std::string_view what)
{
    std::ifstream file(path);

    if(!file.is_open())
    {
        throw DitherError("invalid " + std::string(what) + " - " + path);
    }

    Matrix matrix;
    std::string line;
    std::size_t line_number = 0;

    while(std::getline(file, line))
    {
        line_number++;

        if(!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if(trim(line).empty())
        {
            continue;
        }

        std::vector<double> row;
        std::stringstream ss(line);
        std::string token;

        while(std::getline(ss, token, ','))
        {
            double value = 0.0;

            if(!parse_number(token, value))
            {
                throw DitherError("invalid " + std::string(what) + " value in " + path + " (line " + std::to_string(line_number) + ") - " + token);
            }

            row.push_back(value);
        }

        if(!matrix.empty() && row.size() != matrix[0].size())
        {
            throw DitherError(std::string(what) + " width is not consistent across rows in " + path + ": row 1 has " + std::to_string(matrix[0].size()) +
                              " values, row " + std::to_string(matrix.size() + 1) + " has " + std::to_string(row.size()));
        }

        matrix.push_back(std::move(row));
    }

    if(matrix.empty())
    {
        throw DitherError(std::string(what) + " file is empty - " + path);
    }

    return matrix;
}
