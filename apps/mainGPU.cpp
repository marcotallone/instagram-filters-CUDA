#include <iostream>
#include <vector>

#ifdef ENABLE_CUDA
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

        // Load images from input path
        auto host_images = load_images(input_path);
        if (host_images.empty()) {
            LOG_ERROR("No images found to process");
            std::cout << "No images found in " << input_path.string() << "\n";
            return 1;
        }

        // Process images
        unsigned int success_count = 0U;
        size_t n_images = host_images.size();
        for (auto& host_image : host_images) {
            LOG_INFO("Processing: " + host_image.filename);
            try {
                // Upload to GPU
                auto device_image = gpu_upload(host_image);

                // Launch kernel
                apply_gpu_filter(device_image, filter, delta);

                // Download result back to CPU
                auto output_image =
                    gpu_download(device_image, host_image, filter);

                // Save output image and free memory
                save_image(output_image, output_path / output_image.filename);
                free(output_image.data);  // CPU image ==> malloc()
                gpu_free(device_image);   // GPU image ==> cudaMaloc()

                ++success_count;

            } catch (const std::exception& e) {
                LOG_ERROR("Failed to process " + host_image.filename + ": " +
                          e.what());
                // Continue with next image instead of crashing...
                continue;
            }
        }

        // Final cleanup
        for (auto& host_image : host_images) {
            free_image(host_image);  // calls stbi_image_free()
        }

        LOG_INFO("Finished processing " + std::to_string(success_count) +
                 " out of " + std::to_string(n_images) + " images");
        std::cout << "Finished processing " << success_count << " out of "
                  << n_images << " images\n";
        return 0;

    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: " + std::string(e.what()));
        return 1;
    }
}

#else   // If CUDA is disabled
int main() {
    std::cerr << "CUDA is disabled. Build with -DENABLE_CUDA=ON\n";
    return 1;
}
#endif  // ENABLE_CUDA