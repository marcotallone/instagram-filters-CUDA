#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <tuple>

#include "config.h"
#include "filters.h"
#include "logger.h"

namespace fs = std::filesystem;

std::tuple<fs::path, fs::path, Filter, float> parser(int argc, char* argv[]) {
    // Default values
    fs::path input_path = config::IMAGES_PATH / "input";
    fs::path output_path = config::IMAGES_PATH / "output";
    std::string filter_name = "grayscale";
    Filter filter = Filter::GRAYSCALE;
    float delta = 0.0f;

    // Parse input arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            input_path = argv[++i];
            LOG_INFO("Parsed input path: " + input_path.string());
        } else if (arg == "-o" && i + 1 < argc) {
            output_path = argv[++i];
            LOG_INFO("Parsed output path: " + output_path.string());
        } else if (arg == "-f" && i + 1 < argc) {
            filter_name = argv[++i];
            LOG_INFO("Parsed filter name: " + filter_name);
            // Validate / select correct filter
            if (config::ACCEPTED_FILTERS.find(filter_name) !=
                config::ACCEPTED_FILTERS.end()) {
                if (filter_name == "grayscale") {
                    filter = Filter::GRAYSCALE;
                } else if (filter_name == "sepia") {
                    filter = Filter::SEPIA;
                } else if (filter_name == "brightness") {
                    filter = Filter::BRIGHTNESS;
                }
            } else {
                LOG_ERROR("Invalid filter name: " + filter_name);
                std::cerr << "Invalid filter name: " << filter_name << "\n";
                std::cerr << "Accepted filters: grayscale, sepia, brightness\n";
                exit(1);
            }
        } else if (arg == "-d" && i + 1 < argc) {
            try {
                delta = std::stof(argv[++i]);
                // std::stof() Interprets a floating point value in a string
                LOG_INFO("Parsed delta value: " + std::to_string(delta));
            } catch (const std::invalid_argument& e) {
                LOG_ERROR("Invalid delta value: " + std::string(argv[i]));
                std::cerr << "Invalid delta value: " << argv[i] << "\n";
                exit(1);
            }

        } else {
            LOG_ERROR("Unknown argument: " + arg);
            std::cerr << "Unknown argument: " << arg << "\n";
            std::cerr
                << "Usage: " << argv[0]
                << " [-i input_path] [-o output_path] [-f filter] [-d delta]\n";
            exit(1);
        }
    }

    return {input_path, output_path, filter, delta};
}
