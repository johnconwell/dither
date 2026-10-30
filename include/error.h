#pragma once

#include <stdexcept> // std::runtime_error
#include <string> // std::string

// Thrown by library code for bad input (unknown preset, unreadable file, invalid value...).
// Nothing below main() exits the process: main catches this, prints "Error: <message>"
// and returns EXIT_FAILURE.
class DitherError : public std::runtime_error
{
public:
    explicit DitherError(const std::string& message) : std::runtime_error(message) {}
};
