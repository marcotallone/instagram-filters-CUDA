#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>

#include "imageIO.h"
#include "logger.h"

// CPU grayscale filter
inline Image grayscale_filter(const Image& input_image) {
    // Initialize output image struct
    Image output_image;
    output_image.width = input_image.width;
    output_image.height = input_image.height;
    output_image.channels = input_image.channels;
    // output_image.filename = input_image.filename + "_grayscale";
    output_image.filename =
        input_image.filename.substr(0, input_image.filename.find_last_of(".")) +
        "_grayscale" + input_image.extension;
    output_image.extension = input_image.extension;

    int num_pixels = input_image.width * input_image.height;
    int num_bytes = num_pixels * input_image.channels;

    // Allocate with malloc (STB-compatible)
    output_image.data = (unsigned char*)malloc(num_bytes);
    if (output_image.data == nullptr) {
        LOG_ERROR("Failed to allocate memory for grayscale output");
        throw std::runtime_error(
            "Failed to allocate memory for grayscale output");
    }

    // Apply grayscale filter
    for (int i = 0; i < num_pixels; ++i) {
        unsigned char r = input_image.data[i * input_image.channels + 0];
        unsigned char g = input_image.data[i * input_image.channels + 1];
        unsigned char b = input_image.data[i * input_image.channels + 2];

        float gray = 0.299f * r + 0.587f * g + 0.114f * b;
        unsigned char gray_uchar = static_cast<unsigned char>(gray);

        output_image.data[i * output_image.channels + 0] = gray_uchar;
        output_image.data[i * output_image.channels + 1] = gray_uchar;
        output_image.data[i * output_image.channels + 2] = gray_uchar;
    }

    LOG_INFO("CPU Grayscale filter applied to image: " + input_image.filename);
    return output_image;
}

// CPU sepia filter
inline Image sepia_filter(const Image& input_image) {
    // Initialize output image struct
    Image output_image;
    output_image.width = input_image.width;
    output_image.height = input_image.height;
    output_image.channels = input_image.channels;
    // output_image.filename = input_image.filename + "_sepia";
    output_image.filename =
        input_image.filename.substr(0, input_image.filename.find_last_of(".")) +
        "_sepia" + input_image.extension;
    output_image.extension = input_image.extension;

    int num_pixels = input_image.width * input_image.height;
    int num_bytes = num_pixels * input_image.channels;

    // Allocate with malloc (STB-compatible)
    output_image.data = (unsigned char*)malloc(num_bytes);
    if (output_image.data == nullptr) {
        LOG_ERROR("Failed to allocate memory for grayscale output");
        throw std::runtime_error(
            "Failed to allocate memory for grayscale output");
    }

    // Sepia coefficients
    const float SR = 0.393f, SG = 0.769f, SB = 0.189f;
    const float GR = 0.349f, GG = 0.686f, GB = 0.168f;
    const float BR = 0.272f, BG = 0.534f, BB = 0.131f;

    // Apply sepia filter
    for (int i = 0; i < num_pixels; ++i) {
        unsigned char r = input_image.data[i * input_image.channels + 0];
        unsigned char g = input_image.data[i * input_image.channels + 1];
        unsigned char b = input_image.data[i * input_image.channels + 2];

        float r_sepia = std::min(255.0f, SR * r + SG * g + SB * b);
        float g_sepia = std::min(255.0f, GR * r + GG * g + GB * b);
        float b_sepia = std::min(255.0f, BR * r + BG * g + BB * b);

        output_image.data[i * output_image.channels + 0] =
            static_cast<unsigned char>(r_sepia);
        output_image.data[i * output_image.channels + 1] =
            static_cast<unsigned char>(g_sepia);
        output_image.data[i * output_image.channels + 2] =
            static_cast<unsigned char>(b_sepia);
    }

    LOG_INFO("CPU Sepia filter applied to image: " + input_image.filename);
    return output_image;
}

// CPU brightness filter
inline Image brightness_filter(const Image& input_image, float delta) {
    // Initialize output image struct
    Image output_image;
    output_image.width = input_image.width;
    output_image.height = input_image.height;
    output_image.channels = input_image.channels;
    // output_image.filename = input_image.filename + "_brightness";
    output_image.filename =
        input_image.filename.substr(0, input_image.filename.find_last_of(".")) +
        "_brightness" + input_image.extension;
    output_image.extension = input_image.extension;

    int num_pixels = input_image.width * input_image.height;
    int num_bytes = num_pixels * input_image.channels;

    // Allocate with malloc (STB-compatible)
    output_image.data = (unsigned char*)malloc(num_bytes);
    if (output_image.data == nullptr) {
        LOG_ERROR("Failed to allocate memory for grayscale output");
        throw std::runtime_error(
            "Failed to allocate memory for grayscale output");
    }

    // Clamp delta change to well-defined range
    delta = std::max(-255.0f, std::min(255.0f, delta));

    for (int i = 0; i < num_bytes; ++i) {
        float tmp = input_image.data[i] + delta;
        tmp = std::max(0.0f, std::min(255.0f, tmp));
        output_image.data[i] = static_cast<unsigned char>(tmp);
    }

    LOG_INFO("CPU Brightness filter (delta=" + std::to_string(delta) +
             ") applied to image: " + input_image.filename);
    return output_image;
}

#ifdef ENABLE_CUDA

#include "config.h"
#include "imageIO.h"

// // Struct to hold a pair of GPU buffers for one image (input + output)
struct GpuImage {
    unsigned char* d_input = nullptr;
    unsigned char* d_output = nullptr;
    size_t bytes = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
};

// GPU memory management (implemented in filters.cu)
GpuImage gpu_upload(const Image& image);
Image gpu_download(const GpuImage& gpu_image, const Image& original,
                   const Filter& filter);
void gpu_free(GpuImage& gpu_image);

// GPU filter launcher (implemented in filters.cu)
void apply_gpu_filter(const GpuImage& gpu_image, const Filter& filter_type,
                      float brightness_delta);

#endif  // ENABLE_CUDA