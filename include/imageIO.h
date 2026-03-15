#pragma once
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "config.h"
#include "logger.h"
#include "stb_image.h"
#include "stb_image_write.h"

namespace fs = std::filesystem;

// Simple image struct
struct Image {
    unsigned char* data;  // flat array of pixel data [R, G, B, R, G, B, ...]
    int channels;
    int height;
    int width;
    std::string filename;
    std::string extension;
};

// Loads all images from a directory into a vector
std::vector<Image> load_images(const fs::path& image_dir);

// Free CPU-side image data
void free_image(Image& image);

// Save a filtered image to disk
void save_image(Image& image, fs::path image_path);