#include "error_diffusion.h"

const std::unordered_map<std::pair<int, int>, const double, PairHash> ErrorDiffusion::SCALARS_LINEAR = {
    {R, 1.0}
};

const std::unordered_map<std::pair<int, int>, const double, PairHash> ErrorDiffusion::SCALARS_FLOYD_STEINBERG = {
    {R, 7.0 / 16.0},
    {LD, 3.0 / 16.0}, {D, 5.0 / 16.0}, {RD, 1.0 / 16.0}
};

const std::unordered_map<std::pair<int, int>, const double, PairHash> ErrorDiffusion::SCALARS_JARVICE_JUDICE_NINKE = {
    {R, 7.0 / 48.0}, {RR, 5.0 / 48.0},
    {LLD, 3.0 / 48.0}, {LD, 5.0 / 48.0}, {D, 7.0 / 48.0}, {RD, 5.0 / 48.0}, {RRD, 3.0 / 48.0},
    {LLDD, 1.0 / 48.0}, {LDD, 3.0 / 48.0}, {DD, 5.0 / 48.0}, {RDD, 3.0 / 48.0}, {RRDD, 1.0 / 48.0}
};

const std::unordered_map<std::pair<int, int>, const double, PairHash> ErrorDiffusion::SCALARS_STUCKI = {
    {R, 8.0 / 42.0}, {RR, 4.0 / 42.0},
    {LLD, 2.0 / 42.0}, {LD, 4.0 / 42.0}, {D, 8.0 / 42.0}, {RD, 4.0 / 42.0}, {RRD, 2.0 / 42.0},
    {LLDD, 1.0 / 42.0}, {LDD, 2.0 / 42.0}, {DD, 4.0 / 42.0}, {RDD, 2.0 / 42.0}, {RRDD, 1.0 / 42.0}
};

const std::unordered_map<std::pair<int, int>, const double, PairHash> ErrorDiffusion::SCALARS_ATKINSON = {
    {R, 1.0 / 8.0}, {RR, 1.0 / 8.0},
    {LD, 1.0 / 8.0}, {D, 1.0 / 8.0}, {RD, 1.0 / 8.0},
    {DD, 1.0 / 8.0}
};

const std::unordered_map<std::pair<int, int>, const double, PairHash> ErrorDiffusion::SCALARS_BURKES = {
    {R, 8.0 / 32.0}, {RR, 4.0 / 32.0},
    {LLD, 2.0 / 32.0}, {LD, 4.0 / 32.0}, {D, 8.0 / 32.0}, {RD, 4.0 / 32.0}, {RRD, 2.0 / 32.0}
};

const std::unordered_map<std::pair<int, int>, const double, PairHash> ErrorDiffusion::SCALARS_SIERRA = {
    {R, 5.0 / 32.0}, {RR, 3.0 / 32.0},
    {LLD, 2.0 / 32.0}, {LD, 4.0 / 32.0}, {D, 5.0 / 32.0}, {RD, 4.0 / 32.0}, {RRD, 2.0 / 32.0},
    {LDD, 2.0 / 32.0}, {DD, 3.0 / 32.0}, {RDD, 2.0 / 32.0}
};

const std::unordered_map<std::pair<int, int>, const double, PairHash> ErrorDiffusion::SCALARS_SIERRA_TWO_ROW = {
    {R, 4.0 / 16.0}, {RR, 3.0 / 16.0},
    {LLD, 1.0 / 16.0}, {LD, 2.0 / 16.0}, {D, 3.0 / 16.0}, {RD, 2.0 / 16.0}, {RRD, 1.0 / 16.0}
};

const std::unordered_map<std::pair<int, int>, const double, PairHash> ErrorDiffusion::SCALARS_SIERRA_LITE = {
    {R, 2.0 / 4.0},
    {LD, 1.0 / 4.0}, {D, 1.0 / 4.0}
};

const std::unordered_map<ErrorDiffusionAlgorithm, std::string, EnumHash> ErrorDiffusion::ALGORITHM_STRING = {
    {ErrorDiffusionAlgorithm::LINEAR, "linear"},
    {ErrorDiffusionAlgorithm::FLOYD_STEINBERG, "floyd_steinberg"},
    {ErrorDiffusionAlgorithm::JARVICE_JUDICE_NINKE, "jarvis_judice_ninke"},
    {ErrorDiffusionAlgorithm::STUCKI, "stucki"},
    {ErrorDiffusionAlgorithm::ATKINSON, "atkinson"},
    {ErrorDiffusionAlgorithm::BURKES, "burkes"},
    {ErrorDiffusionAlgorithm::SIERRA, "sierra"},
    {ErrorDiffusionAlgorithm::SIERRA_TWO_ROW, "sierra_two_row"},
    {ErrorDiffusionAlgorithm::SIERRA_LITE, "sierra_lite"},
};

ErrorDiffusion::ErrorDiffusion()
{
    coordinates.resize(0);
    scalars = {{}};
}

ErrorDiffusion::ErrorDiffusion(ErrorDiffusionAlgorithm algorithm)
{
    switch(algorithm)
    {
    case ErrorDiffusionAlgorithm::LINEAR:
        coordinates = COORDINATES_LINEAR;
        scalars = SCALARS_LINEAR;
        break;
    case ErrorDiffusionAlgorithm::FLOYD_STEINBERG:
        coordinates = COORDINATES_FLOYD_STEINBERG;
        scalars = SCALARS_FLOYD_STEINBERG;
        break;
    case ErrorDiffusionAlgorithm::JARVICE_JUDICE_NINKE:
        coordinates = COORDINATES_JARVICE_JUDICE_NINKE;
        scalars = SCALARS_JARVICE_JUDICE_NINKE;
        break;
    case ErrorDiffusionAlgorithm::STUCKI:
        coordinates = COORDINATES_STUCKI;
        scalars = SCALARS_STUCKI;
        break;
    case ErrorDiffusionAlgorithm::ATKINSON:
        coordinates = COORDINATES_ATKINSON;
        scalars = SCALARS_ATKINSON;
        break;
    case ErrorDiffusionAlgorithm::BURKES:
        coordinates = COORDINATES_BURKES;
        scalars = SCALARS_BURKES;
        break;
    case ErrorDiffusionAlgorithm::SIERRA:
        coordinates = COORDINATES_SIERRA;
        scalars = SCALARS_SIERRA;
        break;
    case ErrorDiffusionAlgorithm::SIERRA_TWO_ROW:
        coordinates = COORDINATES_SIERRA_TWO_ROW;
        scalars = SCALARS_SIERRA_TWO_ROW;
        break;
    case ErrorDiffusionAlgorithm::SIERRA_LITE:
        coordinates = COORDINATES_SIERRA_LITE;
        scalars = SCALARS_SIERRA_LITE;
        break;
    default:
        coordinates.resize(0);
        scalars = {{}};
        break;
    }
}

std::string ErrorDiffusion::to_string()
{
    std::string output = "";
    size_t coordinates_size = coordinates.size();

    for(size_t index_coordinates = 0; index_coordinates < coordinates_size; index_coordinates++){
        output += "(" + std::to_string(coordinates[index_coordinates].first) + ", " + std::to_string(coordinates[index_coordinates].second) + "): " + std::to_string(scalars[coordinates[index_coordinates]]) + "\n";
    }

    return output;
}
