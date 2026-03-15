#include <cuda_runtime.h>

#include "logger.h"

// Sepia tone transformation kernel
// Applies a 3x3 color matrix to create warm sepia tones
__global__ void sepia_kernel(unsigned char* d_input, unsigned char* d_output,
                             int width, int height, int channels) {
    // Sepia matrix coefficients
    const float SR = 0.393f, SG = 0.769f, SB = 0.189f;  // Output Red
    const float GR = 0.349f, GG = 0.686f, GB = 0.168f;  // Output Green
    const float BR = 0.272f, BG = 0.534f, BB = 0.131f;  // Output Blue

    // Thread indexing and boundary conditions
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= width * height * channels) return;
    int pixel_idx = idx / channels;

    // Load R, G, B values
    // All 3 threads for this pixel will read the same values
    unsigned char r = d_input[pixel_idx * channels + 0];
    unsigned char g = d_input[pixel_idx * channels + 1];
    unsigned char b = d_input[pixel_idx * channels + 2];

    // Compute sepia R', G', B' values
    // Every thread computes all 3: wasteful but safe!
    float r_sepia = SR * r + SG * g + SB * b;
    float g_sepia = GR * r + GG * g + GB * b;
    float b_sepia = BR * r + BG * g + BB * b;

    // Clamp to [0, 255]
    // if (r_sepia > 255.0f) r_sepia = 255.0f;
    // if (r_sepia < 0.0f) r_sepia = 0.0f;
    // if (g_sepia > 255.0f) g_sepia = 255.0f;
    // if (g_sepia < 0.0f) g_sepia = 0.0f;
    // if (b_sepia > 255.0f) b_sepia = 255.0f;
    // if (b_sepia < 0.0f) b_sepia = 0.0f;
    r_sepia = static_cast<unsigned char>(fminf(255.0f, fmaxf(0.0f, r_sepia)));
    g_sepia = static_cast<unsigned char>(fminf(255.0f, fmaxf(0.0f, g_sepia)));
    b_sepia = static_cast<unsigned char>(fminf(255.0f, fmaxf(0.0f, b_sepia)));

    // Write result based on channel
    // Thread 0 writes red, thread 1 writes green, thread 2 writes blue
    int channel = idx % channels;
    if (channel == 0) {
        d_output[idx] = (unsigned char)r_sepia;
    } else if (channel == 1) {
        d_output[idx] = (unsigned char)g_sepia;
    } else {
        d_output[idx] = (unsigned char)b_sepia;
    }
}

/*
NOTE: alternative kernel using shared variables and synchronization:
__shared__ float shared_sepia[3];

...

// Only thread 0 computes all 3
if (channel == 0) {
    float r = d_input[pixel_idx * 3 + 0];
    float g = d_input[pixel_idx * 3 + 1];
    float b = d_input[pixel_idx * 3 + 2];

    shared_sepia[0] = fminf(255.0f, fmaxf(0.0f, SR*r + SG*g + SB*b));
    shared_sepia[1] = fminf(255.0f, fmaxf(0.0f, GR*r + GG*g + GB*b));
    shared_sepia[2] = fminf(255.0f, fmaxf(0.0f, BR*r + BG*g + BB*b));
}

__syncthreads();  // Wait for thread 0

// All 3 threads read from shared memory
d_output[idx] = (unsigned char)shared_sepia[channel];

...


But: This is actually slower than your current approach due to synchronization
overhead!!!
*/