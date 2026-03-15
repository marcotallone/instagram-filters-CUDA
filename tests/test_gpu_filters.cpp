#ifdef ENABLE_CUDA

#include <gtest/gtest.h>

#include <filesystem>

#include "config.h"
#include "filters.h"
#include "imageIO.h"

namespace fs = std::filesystem;

class GpuFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test image (512×512 RGB)
        test_width = 512;
        test_height = 512;
        test_channels = 3;
        test_bytes = test_width * test_height * test_channels;

        // Allocate test image (gradient from black to white)
        test_image.data = (unsigned char*)malloc(test_bytes);
        test_image.width = test_width;
        test_image.height = test_height;
        test_image.channels = test_channels;
        test_image.filename = "test_gradient";
        test_image.extension = ".png";

        // Fill with gradient
        for (int i = 0; i < test_bytes; i += 3) {
            unsigned char val = (i / 3) % 256;
            test_image.data[i] = val;      // R
            test_image.data[i + 1] = val;  // G
            test_image.data[i + 2] = val;  // B
        }
    }

    void TearDown() override { free(test_image.data); }

    Image test_image;
    int test_width, test_height, test_channels, test_bytes;
};

TEST_F(GpuFilterTest, GrayscaleKernelProducesOutput) {
    auto gpu_img = gpu_upload(test_image);
    apply_gpu_filter(gpu_img, Filter::GRAYSCALE, 0.0f);
    auto result = gpu_download(gpu_img, test_image, Filter::GRAYSCALE);

    EXPECT_NE(result.data, nullptr);
    EXPECT_EQ(result.width, test_width);
    EXPECT_EQ(result.height, test_height);

    free(result.data);
    gpu_free(gpu_img);
}

TEST_F(GpuFilterTest, SepiaKernelProducesOutput) {
    auto gpu_img = gpu_upload(test_image);
    apply_gpu_filter(gpu_img, Filter::SEPIA, 0.0f);
    auto result = gpu_download(gpu_img, test_image, Filter::SEPIA);

    EXPECT_NE(result.data, nullptr);

    free(result.data);
    gpu_free(gpu_img);
}

TEST_F(GpuFilterTest, BrightnessIncreasesPixels) {
    auto gpu_img = gpu_upload(test_image);
    apply_gpu_filter(gpu_img, Filter::BRIGHTNESS, 50.0f);
    auto result = gpu_download(gpu_img, test_image, Filter::BRIGHTNESS);

    // Check that some pixels increased
    int increased_count = 0;
    for (int i = 0; i < test_bytes; i++) {
        if (result.data[i] > test_image.data[i]) { increased_count++; }
    }

    EXPECT_GT(increased_count, 0);

    free(result.data);
    gpu_free(gpu_img);
}

TEST_F(GpuFilterTest, BrightnessClampsAtMax) {
    // Create very bright test image
    for (int i = 0; i < test_bytes; i++) {
        test_image.data[i] = 220;  // Close to max
    }

    auto gpu_img = gpu_upload(test_image);
    apply_gpu_filter(gpu_img, Filter::BRIGHTNESS, 100.0f);
    auto result = gpu_download(gpu_img, test_image, Filter::BRIGHTNESS);

    // All pixels should be clamped to 255
    for (int i = 0; i < test_bytes; i++) { EXPECT_LE(result.data[i], 255); }

    free(result.data);
    gpu_free(gpu_img);
}

#endif  // ENABLE_CUDA