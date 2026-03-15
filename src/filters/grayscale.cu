#include <cuda_runtime.h>

#include "logger.h"

// Grayscale conversion kernel
// Input: RGB image (d_input) with 3 bytes per pixel
// Output: Grayscale image (d_output) with 3 bytes per pixel (replicated Y)
__global__ void grayscale_kernel(unsigned char* d_input,
                                 unsigned char* d_output, int width, int height,
                                 int channels) {
    // Calculate global thread index
    // Each thread handles one BYTE, so we need width*height*channels threads
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // Boundary check
    // If idx is beyond the total number of bytes, exit this thread
    if (idx >= width * height * channels) return;

    // Calculate which PIXEL this byte belongs to
    // A pixel is 3 consecutive bytes: [R, G, B] => integer division
    int pixel_idx = static_cast<int>(idx / channels);

    // Load the R, G, B values for this pixel
    unsigned char r = d_input[pixel_idx * channels + 0];
    unsigned char g = d_input[pixel_idx * channels + 1];
    unsigned char b = d_input[pixel_idx * channels + 2];

    // Apply luminosity formula
    // Y = 0.299*R + 0.587*G + 0.114*B
    // NOTE: cast to float for precision, then back to unsigned char
    float gray = 0.299f * r + 0.587f * g + 0.114f * b;
    unsigned char gray_uchar = static_cast<unsigned char>(gray);
    d_output[idx] = gray_uchar;
}
