#include <cuda_runtime.h>
#include <npp.h>
#include <nppi_arithmetic_and_logical_operations.h>

#include <algorithm>

// #include "filters.cu"

#include "cuda_check.h"
#include "logger.h"

cudaError_t apply_brightness_npp(unsigned char* d_input,
                                 unsigned char* d_output, int width, int height,
                                 float delta) {
    try {
        // Clamp delta to reasonable range [-255, +255]
        float delta_clamped = std::max(-255.0f, std::min(255.0f, delta));

        // Set the Region Of Interest = entire image in this case
        NppiSize roi = {width, height};

        // Calculate stride (bytes per row)
        // NPP typically requires 4-byte aligned strides
        int step_bytes = width * 3;  // 3 bytes per pixel (RGB)

        // Copy input to output (required for later in-place operation)
        // NOTE: this is a GPU to GPU memory copy operation
        CUDA_CHECK(cudaMemcpy(d_output, d_input, width * height * 3,
                              cudaMemcpyDeviceToDevice));

        /*
        NOTE:
        At this point we would use something like:
        'nppiAddC_8u_C3R': Add constant to 3-channel 8-bit image (read mode)
        Signature: nppiAddC_8u_C3R(src, step, constant, dst, step, roi)

        NppStatus status =
            nppiAddC_8u_C3R(d_input,     // Source image pointer
                            step_bytes,  // Source stride (bytes per row)
                            npp_delta,   // Constant to add
                            d_output,    // Destination image pointer
                            step_bytes,  // Destination stride
                            roi          // Region of interest (entire image)
            );

        PROBLEM: nppiAddC_8u_C3R does not clamp values to [0, 255],
                 so if you add 100 to 200, you get 300 → overflow.

                 Therefore we use another in-place solution (below):
        */

        // Use in-place add with AUTOMATIC SATURATION
        if (delta_clamped >= 0.0f) {
            // Brightness increase: use AddC
            Npp8u val = (Npp8u)delta_clamped;
            Npp8u npp_delta[3] = {val, val, val};

            // nppiAddC_8u_C3IR automatically saturates to [0, 255]
            NppStatus status =
                nppiAddC_8u_C3IRSfs(npp_delta, d_output, step_bytes, roi,
                                    0  // Added scale factor
                );

            if (status != NPP_SUCCESS) {
                LOG_ERROR("NPP AddC failed with status: " +
                          std::to_string(status));
                return cudaErrorInvalidValue;
            }

            LOG_DEBUG("Brightness increased by " +
                      std::to_string(delta_clamped));

        } else {
            // Brightness decrease: use SubC
            Npp8u val = (Npp8u)(-delta_clamped);
            Npp8u npp_delta[3] = {val, val, val};

            // nppiSubC_8u_C3IR automatically saturates to [0, 255]
            NppStatus status =
                nppiSubC_8u_C3IRSfs(npp_delta,   // Constant to subtract
                                    d_output,    // Input/Output buffer
                                    step_bytes,  // Stride
                                    roi,         // Region of interest
                                    0            // Added scale factor
                );

            if (status != NPP_SUCCESS) {
                LOG_ERROR("NPP SubC failed with status: " +
                          std::to_string(status));
                return cudaErrorInvalidValue;
            }

            LOG_DEBUG("Brightness decreased by " +
                      std::to_string(-delta_clamped));
        }

        // Synchronize GPU and return
        return cudaDeviceSynchronize();

    } catch (const std::exception& e) {
        LOG_ERROR("NPP brightness exception: " + std::string(e.what()));
        return cudaErrorInvalidValue;
    }
}