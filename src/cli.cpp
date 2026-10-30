#include "cli.h"
#include "cxxopts.hpp"
#include "error.h"
#include "error_diffusion.h"
#include <algorithm> // std::transform
#include <cctype> // std::tolower
#include <charconv> // std::from_chars
#include <filesystem> // std::filesystem::path
#include <iostream> // std::cout
#include <vector> // std::vector

namespace
{
    // the arguments of a list flag such as -r, which must have exactly `expected` of them
    std::vector<std::string> list_arguments(const cxxopts::ParseResult& result, const std::string& flag, std::size_t expected)
    {
        std::vector<std::string> arguments = result[flag].as<std::vector<std::string>>();

        if(arguments.size() != expected)
        {
            throw DitherError("Incorrect number of arguments for --" + flag + " flag. Expected " + std::to_string(expected) + ", received " + std::to_string(arguments.size()) + ".");
        }

        return arguments;
    }

    template <class E>
    E parse_or_throw(const std::string& what, const std::string& text)
    {
        if(const std::optional<E> value = parse_enum<E>(text))
        {
            return *value;
        }

        throw DitherError("Invalid " + what + " - " + text + " (expected one of: " + enum_names<E>() + ")");
    }

    std::size_t parse_frames(const std::string& text)
    {
        const char* first = text.data();
        const char* last = first + text.size();
        std::size_t frames = 0;
        const auto [ptr, ec] = std::from_chars(first, last, frames);

        if(text.empty() || ec != std::errc() || ptr != last || frames < 1 || frames > CLI::TEMPORAL_FRAMES_MAX)
        {
            throw DitherError("Invalid number of temporal frames - " + text + " (expected a whole number from 1 to " + std::to_string(CLI::TEMPORAL_FRAMES_MAX) + ")");
        }

        return frames;
    }
}

void CLI::parse(int argc, const char* argv[])
{
    cxxopts::Options options("dither", "CLI image processing tool");

    options.custom_help("-i <INPUT_FILE_PATH> -o <OUTPUT_FILE_PATH> [OPTIONS...]");
    options.set_width(80);
    options.set_tab_expansion();

    options.add_options()
        ("h,help", "Displays help")
        ("i,input", "Specifies the input png image file path", cxxopts::value<std::string>(), "<FILE_PATH>")
        ("o,output", "Specifies the file path the output png will be saved to", cxxopts::value<std::string>(), "<FILE_PATH>")
        ("a,gamma_correction", "Processes the image in linear light (gamma correction); applies to -g, -c, -r, -e, -d and -t")
        ("g,grayscale", "Converts the image to grayscale using the specified channel weights", cxxopts::value<std::string>(), "<GRAYSCALE_WEIGHTS>")
        ("c,convolve", "Convolves the input image using the specified kernel", cxxopts::value<std::string>(), "<KERNEL>")
        ("r,reduce", "Reduces the image to the specified palette without applying dithering", cxxopts::value<std::vector<std::string>>(), "<MAPPING_METHOD>,<PALETTE>")
        ("e,error_diffusion", "Dithers the image using the specified error diffusion algorithm and color mapping method", cxxopts::value<std::vector<std::string>>(), "<ERROR_DIFFUSION_ALGORITHM>,<SCAN_PATTERN>,<MAPPING_METHOD>,<PALETTE>")
        ("d,ordered", "Dithers the image using the specified threshold matrix and color mapping method", cxxopts::value<std::vector<std::string>>(), "<ORDERED_THRESHOLD_MATRIX>,<MAPPING_METHOD>,<PALETTE>")
        ("t,temporal", "Dithers the image over time using the specified temporal dithering method and color mapping method", cxxopts::value<std::vector<std::string>>(), "<TEMPORAL_METHOD>,<TEMPORAL_FRAMES>,<MAPPING_METHOD>,<PALETTE>")
        ("b,benchmark", "Prints the time taken to load, process and save the image")
    ;

    cxxopts::ParseResult result;

    try
    {
        result = options.parse(argc, argv);
    }
    catch(const cxxopts::exceptions::exception& e)
    {
        throw DitherError(std::string("parsing options: ") + e.what());
    }

    if(result.count("help"))
    {
        std::cout << options.help() << std::endl;
        help = true;
        return;
    }

    if(!result.count("input"))
    {
        throw DitherError("No input image file path specified");
    }

    if(!result.count("output"))
    {
        throw DitherError("No output image file path specified");
    }

    file_path_input = result["input"].as<std::string>();
    file_path_output = result["output"].as<std::string>();
    gamma_correction = result.count("gamma_correction") > 0;
    benchmark = result.count("benchmark") > 0;

    if(result.count("grayscale"))
    {
        grayscale = true;
        grayscale_weights = result["grayscale"].as<std::string>();
    }

    if(result.count("convolve"))
    {
        convolve = true;
        convolve_kernel = result["convolve"].as<std::string>();
    }

    // only one quantising operation per run; -g and -c are pre-processing and combine freely
    const std::size_t quantisers = result.count("reduce") + result.count("error_diffusion") + result.count("ordered") + result.count("temporal");

    if(quantisers > 1)
    {
        throw DitherError("Only one of -r, -e, -d and -t can be used per run (-g and -c can be combined with any of them)");
    }

    // the raw mapping method text of the one quantising operation
    std::string mapping_method_text;

    if(result.count("reduce"))
    {
        const auto arguments = list_arguments(result, "reduce", 2);
        reduce = true;
        mapping_method_text = arguments[0];
        palette = arguments[1];
    }

    std::string scan_pattern_text;

    if(result.count("error_diffusion"))
    {
        const auto arguments = list_arguments(result, "error_diffusion", 4);
        error_diffusion = true;
        error_diffusion_algorithm = arguments[0];
        scan_pattern_text = arguments[1];
        mapping_method_text = arguments[2];
        palette = arguments[3];
    }

    if(result.count("ordered"))
    {
        const auto arguments = list_arguments(result, "ordered", 3);
        ordered = true;
        ordered_threshold_matrix = arguments[0];
        mapping_method_text = arguments[1];
        palette = arguments[2];
    }

    std::string temporal_method_text;
    std::string temporal_frames_text;

    if(result.count("temporal"))
    {
        const auto arguments = list_arguments(result, "temporal", 4);
        temporal = true;
        temporal_method_text = arguments[0];
        temporal_frames_text = arguments[1];
        mapping_method_text = arguments[2];
        palette = arguments[3];
    }

    // output format comes from the extension (case-insensitive); .gif works for every operation
    std::string extension = std::filesystem::path(file_path_output).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if(extension != ".png" && extension != ".gif")
    {
        throw DitherError("Unsupported output file format - " + file_path_output + " (expected .png or .gif)");
    }

    if(error_diffusion)
    {
        if(!ErrorDiffusion::has_preset(error_diffusion_algorithm))
        {
            throw DitherError("invalid error diffusion algorithm - " + error_diffusion_algorithm);
        }

        error_diffusion_scan_pattern = parse_or_throw<ScanPattern>("scan pattern", scan_pattern_text);
    }

    if(temporal)
    {
        temporal_method = parse_or_throw<TemporalMethod>("temporal dithering method", temporal_method_text);

        // temporal dithering picks colours by per-pixel distance, so UNIFORM_HISTOGRAM is not valid with -t
        const std::optional<MappingMethod> method = parse_enum<MappingMethod>(mapping_method_text);

        if(!method || *method == MappingMethod::UNIFORM_HISTOGRAM)
        {
            throw DitherError("Invalid mapping method for --temporal - " + mapping_method_text + " (expected one of: EUCLIDEAN_DISTANCE, MANHATTAN_DISTANCE)");
        }

        mapping_method = *method;
        temporal_frames = parse_frames(temporal_frames_text);
    }
    else if(reduce || error_diffusion || ordered)
    {
        mapping_method = parse_or_throw<MappingMethod>("mapping method", mapping_method_text);
    }
}

std::string CLI::to_string() const
{
    auto flag = [](bool value) { return value ? std::string("true") : std::string("false"); };
    std::string output;
    output += "file_path_input: " + file_path_input + "\n";
    output += "file_path_output: " + file_path_output + "\n";
    output += "gamma_correction: " + flag(gamma_correction) + "\n";
    output += "grayscale: " + flag(grayscale) + "\n";
    output += "grayscale_weights: " + grayscale_weights + "\n";
    output += "convolve: " + flag(convolve) + "\n";
    output += "convolve_kernel: " + convolve_kernel + "\n";
    output += "reduce: " + flag(reduce) + "\n";
    output += "error_diffusion: " + flag(error_diffusion) + "\n";
    output += "error_diffusion_algorithm: " + error_diffusion_algorithm + "\n";
    output += "error_diffusion_scan_pattern: " + std::string(enum_name(error_diffusion_scan_pattern)) + "\n";
    output += "ordered: " + flag(ordered) + "\n";
    output += "ordered_threshold_matrix: " + ordered_threshold_matrix + "\n";
    output += "temporal: " + flag(temporal) + "\n";
    output += "temporal_method: " + std::string(enum_name(temporal_method)) + "\n";
    output += "temporal_frames: " + std::to_string(temporal_frames) + "\n";
    output += "mapping_method: " + std::string(enum_name(mapping_method)) + "\n";
    output += "palette: " + palette + "\n";
    output += "benchmark: " + flag(benchmark) + "\n";
    return output;
}
