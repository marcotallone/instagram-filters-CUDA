#pragma once

#ifdef ENABLE_CUDA

#include <cuda_runtime.h>

#include <stdexcept>
#include <string>

#include "logger.h"

// Helper macro: checks a CUDA call and throws a descriptive error on failure
// This is the GPU equivalent of checking errno after a system call
#define CUDA_CHECK(call)                                                       \
    do {                                                                       \
        cudaError_t err = (call);                                              \
        if (err != cudaSuccess) {                                              \
            std::string msg = std::string("CUDA error in ") + __FILE__ + ":" + \
                              std::to_string(__LINE__) + " → " +               \
                              cudaGetErrorString(err);                         \
            LOG_ERROR(msg);                                                    \
            throw std::runtime_error(msg);                                     \
        }                                                                      \
    } while (0)

#endif  // ENABLE_CUDA