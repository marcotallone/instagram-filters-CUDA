#include <cuda_runtime.h>

#include <stdexcept>
#include <string>

#include "config.h"
#include "cuda_check.h"
#include "filters.h"
#include "imageIO.h"
#include "logger.h"

// // Struct to hold a pair of GPU buffers for one image (input + output)
// struct GpuImage {
//     unsigned char* d_input = nullptr;   // device INPUT image original pixels
//     unsigned char* d_output = nullptr;  // device OUTPUT imagefiltered pixels
//     size_t bytes = 0;                   // total image buffer size in bytes
//     int width = 0;
//     int height = 0;
//     int channels = 0;
// };

// Grayscale image kernel
__global__ void grayscale_kernel(unsigned char* d_input,
                                 unsigned char* d_output, int width, int height,
                                 int channels);

// Sepia image kernel
__global__ void sepia_kernel(unsigned char* d_input, unsigned char* d_output,
                             int width, int height, int channels);

// Brightness image (NPP)
cudaError_t apply_brightness_npp(unsigned char* d_input,
                                 unsigned char* d_output, int width, int height,
                                 float delta);

// Upload image to GPU memory
GpuImage gpu_upload(const Image& image) {
    // Create GPU image container
    GpuImage gpu_image;

    // Calculate total image bytes and other metadata
    gpu_image.bytes =
        image.width * image.height * image.channels * sizeof(unsigned char);
    gpu_image.width = image.width;
    gpu_image.height = image.height;
    gpu_image.channels = image.channels;

    // Allocate GPU memory for both INPUT and OUTPUT images
    CUDA_CHECK(cudaMalloc(&gpu_image.d_input, gpu_image.bytes));
    CUDA_CHECK(cudaMalloc(&gpu_image.d_output, gpu_image.bytes));

    // Copy input image data to the device input buffer
    CUDA_CHECK(cudaMemcpy(gpu_image.d_input, image.data, gpu_image.bytes,
                          cudaMemcpyHostToDevice));

    LOG_INFO("Uploaded image " + image.filename + " to GPU (" +
             std::to_string(gpu_image.bytes) + " bytes)");

    return gpu_image;
}

// Copy result back to host memory
Image gpu_download(const GpuImage& gpu_image, const Image& original,
                   const Filter& filter) {
    // Get filter name based on actual filter used
    std::string filter_name;
    switch (filter) {
        case Filter::GRAYSCALE:
            filter_name = "grayscale";
            break;
        case Filter::SEPIA:
            filter_name = "sepia";
            break;
        case Filter::BRIGHTNESS:
            filter_name = "brightness";
            break;
        default:
            throw std::runtime_error("Unknown filter type in gpu_download");
    }

    // Catch any silent kernel launch failures (like architecture mismatches)
    CUDA_CHECK(cudaGetLastError());

    CUDA_CHECK(cudaDeviceSynchronize());

    // Create OUTPUT image on host
    Image result;
    result.filename =
        original.filename.substr(0, original.filename.find_last_of(".")) + "_" +
        filter_name + original.extension;
    result.width = gpu_image.width;
    result.height = gpu_image.height;
    result.channels = gpu_image.channels;
    result.extension = original.extension;

    // Allocate host memory to store result
    result.data = (unsigned char*)malloc(gpu_image.bytes);
    if (result.data == nullptr) {
        throw std::runtime_error("Failed to allocate memory for gpu_download");
    }

    // Copy processed output from memory to host
    CUDA_CHECK(cudaMemcpy(result.data, gpu_image.d_output, gpu_image.bytes,
                          cudaMemcpyDeviceToHost));

    LOG_INFO("Downloaded filtered image " + result.filename + " from GPU (" +
             std::to_string(gpu_image.bytes) + " bytes)");

    return result;
}

// Free memory
void gpu_free(GpuImage& gpu_image) {
    // Always check before freeing (defensive programming)
    // cudaFree(nullptr) is technically safe, but explicit is better

    if (gpu_image.d_input != nullptr) {
        CUDA_CHECK(cudaFree(gpu_image.d_input));
        gpu_image.d_input = nullptr;
    }

    if (gpu_image.d_output != nullptr) {
        CUDA_CHECK(cudaFree(gpu_image.d_output));
        gpu_image.d_output = nullptr;
    }

    gpu_image.bytes = 0;

    LOG_DEBUG("Freed GPU memory");
}

// Kernel launcher
void apply_gpu_filter(const GpuImage& gpu_image, const Filter& filter_type,
                      float brightness_delta) {
    // Compute threads and blocks for the kernel
    int n_threads = 256;
    int n_blocks = (gpu_image.bytes + n_threads - 1) / n_threads;
    // OR:
    // int n_blocks = cuda::ceil_div(gpu_image.bytes, n_threads);

    // Apply selected filter type
    switch (filter_type) {
        case Filter::GRAYSCALE:
            LOG_INFO("Launching grayscale kernel");
            grayscale_kernel<<<n_blocks, n_threads>>>(
                gpu_image.d_input, gpu_image.d_output, gpu_image.width,
                gpu_image.height, gpu_image.channels);
            break;

        case Filter::SEPIA:
            LOG_INFO("Launching sepia kernel");
            sepia_kernel<<<n_blocks, n_threads>>>(
                gpu_image.d_input, gpu_image.d_output, gpu_image.width,
                gpu_image.height, gpu_image.channels);
            break;

        case Filter::BRIGHTNESS:
            LOG_INFO("Applying brightness adjustment (delta=" +
                     std::to_string(brightness_delta) + ")");
            CUDA_CHECK(apply_brightness_npp(
                gpu_image.d_input, gpu_image.d_output, gpu_image.width,
                gpu_image.height, brightness_delta));
            break;
    }

    CUDA_CHECK(cudaDeviceSynchronize());
}
