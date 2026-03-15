#include <chrono>
#include <iostream>
#include <vector>

#include "config.h"
#include "filters.h"
#include "imageIO.h"
#include "logger.h"
#include "parser.h"

int main(int argc, char* argv[]) {
    init_logger();

    try {
        // Parse command line arguments
        auto [input_path, output_path, filter, delta] = parser(argc, argv);

        LOG_INFO("=== CPU Image Filter Pipeline ===");
        LOG_INFO("Input path:  " + input_path.string());
        LOG_INFO("Output path: " + output_path.string());

        // Load images from input path
        auto host_images = load_images(input_path);
        if (host_images.empty()) {
            LOG_ERROR("No images found to process");
            std::cout << "No images found in " << input_path.string() << "\n";
            return 1;
        }

        LOG_INFO("Loaded " + std::to_string(host_images.size()) +
                 " images for processing");

        // Process images
        unsigned int success_count = 0U;
        size_t n_images = host_images.size();

        for (auto& host_image : host_images) {
            LOG_INFO("Processing: " + host_image.filename);

            try {
                // Start timer
                auto start_time = std::chrono::high_resolution_clock::now();

                Image output_image;

                // Apply selected filter
                switch (filter) {
                    case Filter::GRAYSCALE:
                        LOG_DEBUG("Applying GRAYSCALE filter");
                        output_image = grayscale_filter(host_image);
                        break;

                    case Filter::SEPIA:
                        LOG_DEBUG("Applying SEPIA filter");
                        output_image = sepia_filter(host_image);
                        break;

                    case Filter::BRIGHTNESS:
                        LOG_DEBUG("Applying BRIGHTNESS filter (delta=" +
                                  std::to_string(delta) + ")");
                        output_image = brightness_filter(host_image, delta);
                        break;

                    default:
                        LOG_ERROR("Unknown filter type");
                        throw std::runtime_error("Unknown filter type");
                }

                // End timer
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time - start_time);

                LOG_DEBUG("Filter execution time: " +
                          std::to_string(duration.count()) + " ms");

                // Save output image
                save_image(output_image, output_path / output_image.filename);

                // Free the CPU-allocated output image
                free(output_image.data);

                ++success_count;

                std::cout << "✓ " << host_image.filename << " -> "
                          << output_image.filename << "\n";

            } catch (const std::exception& e) {
                LOG_ERROR("Failed to process " + host_image.filename + ": " +
                          e.what());
                std::cout << "✗ " << host_image.filename << " (error)\n";
                continue;
            }
        }

        // Final cleanup
        for (auto& host_image : host_images) {
            free_image(host_image);  // calls stbi_image_free()
        }

        LOG_INFO("Finished processing " + std::to_string(success_count) +
                 " out of " + std::to_string(n_images) + " images");
        std::cout << "\n✅ Finished processing " << success_count << " out of "
                  << n_images << " images\n";
        std::cout << "Output saved to: " << output_path.string() << "\n";

        return 0;

    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: " + std::string(e.what()));
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}