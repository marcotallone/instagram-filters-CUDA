#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// Suppress sprintf deprecation warning (from STB library only)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "imageIO.h"
#pragma clang diagnostic pop

std::vector<Image> load_images(const fs::path& image_dir) {
    if (!fs::is_directory(image_dir)) {
        LOG_ERROR("Provided 'image_dir' path is not a directory: " +
                  image_dir.string());
        throw std::runtime_error(
            "Provided 'image_dir' path is not a directory: " +
            image_dir.string());
    }

    if (!fs::exists(image_dir)) {
        LOG_ERROR("Images directory does not exist: " + image_dir.string());
        throw std::runtime_error("Images directory does not exist: " +
                                 image_dir.string());
    }

    // Load images and store them in a vector of Images
    // NOTE: only .png, .jpg, .jpeg image extension will be loaded
    std::vector<Image> images;
    for (const auto& file : fs::directory_iterator(image_dir)) {
        std::string ext = file.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (config::ACCEPTED_EXT.find(ext) == config::ACCEPTED_EXT.end()) {
            LOG_DEBUG("Unsupported image format: " + ext +
                      " for file: " + file.path().filename().string());
            continue;
        }

        Image image;
        image.filename = file.path().filename().string();
        image.extension = ext;

        // Use stbi_load() to fill img.data, img.width, img.height, img.channels
        // Signature: stbi_load(path, &w, &h, &channels, desired_channels)
        image.data = stbi_load(file.path().c_str(), &image.width, &image.height,
                               &image.channels, 3);

        if (image.data == nullptr) {
            LOG_ERROR("Failed to load: " + image.filename);
            continue;
        }

        // Force channels to 3 since we explicitly requested 3 components per
        // pixel
        image.channels = 3;

        LOG_DEBUG("Loaded: " + image.filename + " [" +
                  std::to_string(image.width) + "x" +
                  std::to_string(image.height) + "]");
        images.push_back(image);
    }
    LOG_INFO("Loaded " + std::to_string(images.size()) + " images");
    return images;
}

void free_image(Image& image) {
    if (image.data != nullptr) {
        stbi_image_free(image.data);
        image.data = nullptr;
    }
}

void save_image(Image& image, fs::path image_path) {
    // Parent output folder (create if nonexistent)
    fs::path output_dir = image_path.parent_path();
    if (!fs::exists(output_dir)) { fs::create_directories(output_dir); }
    LOG_INFO("Created output directory: " + output_dir.string());

    // Save image
    std::string output_path = image_path.string();
    int success = stbi_write_png(output_path.c_str(), image.width, image.height,
                                 image.channels, image.data,
                                 0  // stride (0 = tightly packed)
    );

    if (success) {
        LOG_INFO("Saved: " + image.filename + " to " + output_path);
    } else {
        LOG_ERROR("Failed to save: " + output_path);
    }
}