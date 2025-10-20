#include "cli.h"

CLI::CLI()
{
    file_path_input = "";
    file_path_output = "";
    gamma_correction = false;
    grayscale = false;
    grayscale_weights = "";
    convolve = false;
    convolve_kernel = "";
    reduce = false;
    error_diffusion = false;
    error_diffusion_algorithm = "";
    ordered = false;
    ordered_threshold_matrix = "";
    temporal = false;
    temporal_method = "";
    temporal_frames = "";
    mapping_method = "";
    palette = "";
    benchmark = false;
    return;
}

bool CLI::parse(int argc, const char* argv[])
{
    try
    {
        cxxopts::Options options("dither", "CLI image processing tool");

        options.custom_help("-i <INPUT_FILE_PATH> -o <OUTPUT_FILE_PATH> [OPTIONS...]");
        options.set_width(80);
        options.set_tab_expansion();

        options.add_options()
            ("h,help", "Displays help")
            ("i,input", "Specifies the input png image file path", cxxopts::value<std::string>(), "<FILE_PATH>")
            ("o,output", "Specifies the file path the output png will be saved to", cxxopts::value<std::string>(), "<FILE_PATH>")
            ("a,gamma_correction", "Applies gamma correction the the input image")
            ("g,grayscale", "Applies gamma correction the the input image", cxxopts::value<std::string>(), "<GRAYSCALE_WEIGHTS>")
            ("c,convolve", "Convolves the input image using the specified kernel", cxxopts::value<std::string>(), "<KERNEL>")
            ("r,reduce", "Reduces the image to the specified palette without applying dithering", cxxopts::value<std::vector<std::string>>(), "<MAPPING_METHOD>,<PALETTE>")
            ("e,error_diffusion", "Dithers the image using the specified error diffusion algorithm and color mapping method", cxxopts::value<std::vector<std::string>>(), "<ERROR_DIFFUSION_ALGORITHM>,<SCAN_PATTERN>,<MAPPING_METHOD>,<PALETTE>")
            ("d,ordered", "Dithers the image using the specified threshold matrix and color mapping method", cxxopts::value<std::vector<std::string>>(), "<ORDERED_THRESHOLD_MATRIX>,<MAPPING_METHOD>,<PALETTE>")
            ("t,temporal", "Dithers the image over time using the specified temporal dithering method and color mapping method", cxxopts::value<std::vector<std::string>>(), "<TEMPORAL_METHOD>,<TEMPORAL_FRAMES>,<MAPPING_METHOD>,<PALETTE>")
            ("b,benchmark", "Displays benchmark information to stdout")
        ;

        cxxopts::ParseResult result = options.parse(argc, argv);

        if(result.count("help"))
        {
            std::cout << options.help() << std::endl;
            std::exit(EXIT_SUCCESS);
        }

        if(result.count("input"))
        {
            file_path_input = result["input"].as<std::string>();
        }
        else
        {
            std::cerr << "Error: No input image file path specified" << std::endl;
            return false;
        }

        if(result.count("output"))
        {
            file_path_output = result["output"].as<std::string>();
        }
        else
        {
            std::cerr << "Error: No output image file path specified" << std::endl;
            return false;
        }

        if(result.count("gamma_correction"))
        {
            gamma_correction = true;
        }

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

        if(result.count("reduce"))
        {
            std::size_t num_args = result["reduce"].as<std::vector<std::string>>().size();

            if(num_args != 2)
            {
                std::cerr << "Error: Incorrect number of arguments for --reduce flag. Expected 2, received " << num_args << "." << std::endl;
                std::cerr << result["reduce"].as<std::vector<std::string>>()[0] << " " << result["reduce"].as<std::vector<std::string>>()[1] << std::endl;
                return false;
            }

            reduce = true;
            mapping_method = result["reduce"].as<std::vector<std::string>>()[0];
            palette = result["reduce"].as<std::vector<std::string>>()[1];
        }

        if(result.count("error_diffusion"))
        {
            std::size_t num_args = result["error_diffusion"].as<std::vector<std::string>>().size();

            if(num_args != 4)
            {
                std::cerr << "Error: Incorrect number of arguments for --error_diffusion flag. Expected 4, received " << num_args << "." << std::endl;
                return false;
            }

            error_diffusion = true;
            error_diffusion_algorithm = result["error_diffusion"].as<std::vector<std::string>>()[0];
            error_diffusion_scan_pattern = result["error_diffusion"].as<std::vector<std::string>>()[1];
            mapping_method = result["error_diffusion"].as<std::vector<std::string>>()[2];
            palette = result["error_diffusion"].as<std::vector<std::string>>()[3];
        }

        if(result.count("ordered"))
        {
            std::size_t num_args = result["ordered"].as<std::vector<std::string>>().size();

            if(num_args != 3)
            {
                std::cerr << "Error: Incorrect number of arguments for --ordered flag. Expected 3, received " << num_args << "." << std::endl;
                return false;
            }

            ordered = true;
            ordered_threshold_matrix = result["ordered"].as<std::vector<std::string>>()[0];
            mapping_method = result["ordered"].as<std::vector<std::string>>()[1];
            palette = result["ordered"].as<std::vector<std::string>>()[2];
        }

        if(result.count("temporal"))
        {
            std::size_t num_args = result["temporal"].as<std::vector<std::string>>().size();

            if(num_args != 4)
            {
                std::cerr << "Error: Incorrect number of arguments for --temporal flag. Expected 4, received " << num_args << "." << std::endl;
                return false;
            }

            temporal = true;
            temporal_method = result["temporal"].as<std::vector<std::string>>()[0];
            temporal_frames = result["temporal"].as<std::vector<std::string>>()[1];
            mapping_method = result["temporal"].as<std::vector<std::string>>()[2];
            palette = result["temporal"].as<std::vector<std::string>>()[3];
        }

        if(result.count("benchmark"))
        {
            benchmark = true;
        }
    }
    catch (const cxxopts::exceptions::exception& e)
    {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return false;
    }

    return true;
}

std::string CLI::to_string()
{
    std::string output = "";
    output += "file_path_input: " + file_path_input + "\n";
    output += "file_path_output: " + file_path_output + "\n";
    output += "gamma_correction: ";
    output += gamma_correction ? "true\n" : "false\n";
    output += "grayscale: ";
    output += grayscale ? "true\n" : "false\n";
    output += "grayscale_weights: ";
    output += grayscale_weights + "\n";
    output += "convolve: ";
    output += convolve ? "true\n" : "false\n";
    output += "convolve kernel: ";
    output += convolve_kernel + "\n";
    output += "reduce: ";
    output += reduce ? "true\n" : "false\n";
    output += "error_diffusion: ";
    output += error_diffusion ? "true\n" : "false\n";
    output += "error_diffusion_algorithm: " + error_diffusion_algorithm + "\n";
    output += "error_diffusion_scan_pattern: " + error_diffusion_scan_pattern + "\n";
    output += "ordered: ";
    output += ordered ? "true\n" : "false\n";
    output += "ordered_threshold_matrix: " + ordered_threshold_matrix + "\n";
    output += temporal ? "true\n" : "false\n";
    output += "temporal_method: " + temporal_method + "\n";
    output += "temporal_frames: " + temporal_frames + "\n";
    output += "mapping_method: " + mapping_method + "\n";
    output += "palette: " + palette + "\n";
    output += "benchmark: ";
    output += benchmark ? "true\n" : "false\n";
    return output;
}
