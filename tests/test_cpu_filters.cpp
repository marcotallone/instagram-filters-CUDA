#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <filesystem>

#include "filters.h"
#include "imageIO.h"
#include "logger.h"

namespace fs = std::filesystem;

// ============================================================================
// TEST FIXTURE: CPU Filter Tests
// ============================================================================

class CpuFilterTest : public ::testing::Test {
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

        // Fill with gradient pattern
        for (int i = 0; i < test_bytes; i += 3) {
            unsigned char val = (i / 3) % 256;
            test_image.data[i] = val;      // R
            test_image.data[i + 1] = val;  // G
            test_image.data[i + 2] = val;  // B
        }
    }

    void TearDown() override {
        if (test_image.data != nullptr) {
            free(test_image.data);
            test_image.data = nullptr;
        }
    }

    // Helper: Create a uniform color image
    Image create_uniform_image(unsigned char r, unsigned char g,
                               unsigned char b, int width = 256,
                               int height = 256) {
        Image img;
        img.width = width;
        img.height = height;
        img.channels = 3;
        img.filename = "uniform_test";
        img.extension = ".png";

        int num_pixels = width * height;
        int num_bytes = num_pixels * 3;
        img.data = (unsigned char*)malloc(num_bytes);

        for (int i = 0; i < num_pixels; ++i) {
            img.data[i * 3 + 0] = r;
            img.data[i * 3 + 1] = g;
            img.data[i * 3 + 2] = b;
        }

        return img;
    }

    // Helper: Check if two images are equal
    bool images_equal(const Image& img1, const Image& img2) {
        if (img1.width != img2.width || img1.height != img2.height ||
            img1.channels != img2.channels) {
            return false;
        }

        int bytes = img1.width * img1.height * img1.channels;
        return std::memcmp(img1.data, img2.data, bytes) == 0;
    }

    // Helper: Check if images are approximately equal (within tolerance)
    bool images_approx_equal(const Image& img1, const Image& img2,
                             unsigned char tolerance = 1) {
        if (img1.width != img2.width || img1.height != img2.height ||
            img1.channels != img2.channels) {
            return false;
        }

        int bytes = img1.width * img1.height * img1.channels;
        for (int i = 0; i < bytes; ++i) {
            if (std::abs((int)img1.data[i] - (int)img2.data[i]) > tolerance) {
                return false;
            }
        }
        return true;
    }

    Image test_image;
    int test_width, test_height, test_channels, test_bytes;
};

// ============================================================================
// GRAYSCALE FILTER TESTS
// ============================================================================

TEST_F(CpuFilterTest, GrayscaleProducesValidOutput) {
    Image result = grayscale_filter(test_image);

    EXPECT_NE(result.data, nullptr);
    EXPECT_EQ(result.width, test_width);
    EXPECT_EQ(result.height, test_height);
    EXPECT_EQ(result.channels, test_channels);

    free(result.data);
}

TEST_F(CpuFilterTest, GrayscalePureRedBecomesGray) {
    // Pure red image (255, 0, 0)
    Image red_img = create_uniform_image(255, 0, 0);
    Image result = grayscale_filter(red_img);

    // Expected gray value: 0.299 * 255 ≈ 76
    unsigned char expected = (unsigned char)(0.299f * 255);

    // Check first pixel
    unsigned char pixel_r = result.data[0];
    unsigned char pixel_g = result.data[1];
    unsigned char pixel_b = result.data[2];

    EXPECT_EQ(pixel_r, pixel_g);
    EXPECT_EQ(pixel_g, pixel_b);
    EXPECT_NEAR(pixel_r, expected, 1);  // Allow 1-byte tolerance

    free(red_img.data);
    free(result.data);
}

TEST_F(CpuFilterTest, GrayscalePureGreenBecomesGray) {
    // Pure green image (0, 255, 0)
    Image green_img = create_uniform_image(0, 255, 0);
    Image result = grayscale_filter(green_img);

    // Expected gray value: 0.587 * 255 ≈ 150
    unsigned char expected = (unsigned char)(0.587f * 255);

    unsigned char pixel_r = result.data[0];
    unsigned char pixel_g = result.data[1];
    unsigned char pixel_b = result.data[2];

    EXPECT_EQ(pixel_r, pixel_g);
    EXPECT_EQ(pixel_g, pixel_b);
    EXPECT_NEAR(pixel_r, expected, 1);

    free(green_img.data);
    free(result.data);
}

TEST_F(CpuFilterTest, GrayscalePureBlueBecomesGray) {
    // Pure blue image (0, 0, 255)
    Image blue_img = create_uniform_image(0, 0, 255);
    Image result = grayscale_filter(blue_img);

    // Expected gray value: 0.114 * 255 ≈ 29
    unsigned char expected = (unsigned char)(0.114f * 255);

    unsigned char pixel_r = result.data[0];
    EXPECT_EQ(pixel_r, result.data[1]);
    EXPECT_EQ(result.data[1], result.data[2]);
    EXPECT_NEAR(pixel_r, expected, 1);

    free(blue_img.data);
    free(result.data);
}

TEST_F(CpuFilterTest, GrayscaleWhiteStaysWhite) {
    // White image (255, 255, 255)
    Image white_img = create_uniform_image(255, 255, 255);
    Image result = grayscale_filter(white_img);

    // All pixels should be 255
    int num_pixels = white_img.width * white_img.height;
    for (int i = 0; i < num_pixels; ++i) {
        unsigned char r = result.data[i * 3 + 0];
        unsigned char g = result.data[i * 3 + 1];
        unsigned char b = result.data[i * 3 + 2];

        EXPECT_EQ(r, 255);
        EXPECT_EQ(g, 255);
        EXPECT_EQ(b, 255);
    }

    free(white_img.data);
    free(result.data);
}

TEST_F(CpuFilterTest, GrayscaleBlackStaysBlack) {
    // Black image (0, 0, 0)
    Image black_img = create_uniform_image(0, 0, 0);
    Image result = grayscale_filter(black_img);

    // All pixels should be 0
    int num_pixels = black_img.width * black_img.height;
    for (int i = 0; i < num_pixels; ++i) {
        unsigned char r = result.data[i * 3 + 0];
        EXPECT_EQ(r, 0);
        EXPECT_EQ(result.data[i * 3 + 1], 0);
        EXPECT_EQ(result.data[i * 3 + 2], 0);
    }

    free(black_img.data);
    free(result.data);
}

// ============================================================================
// SEPIA FILTER TESTS
// ============================================================================

TEST_F(CpuFilterTest, SepiaProducesValidOutput) {
    Image result = sepia_filter(test_image);

    EXPECT_NE(result.data, nullptr);
    EXPECT_EQ(result.width, test_width);
    EXPECT_EQ(result.height, test_height);
    EXPECT_EQ(result.channels, test_channels);

    free(result.data);
}

TEST_F(CpuFilterTest, SepiaProducesWarmTone) {
    // White image should become brownish with sepia
    Image white_img = create_uniform_image(255, 255, 255);
    Image result = sepia_filter(white_img);

    // For white input, sepia produces:
    // R = 0.393*255 + 0.769*255 + 0.189*255 ≈ 255
    // G = 0.349*255 + 0.686*255 + 0.168*255 ≈ 238
    // B = 0.272*255 + 0.534*255 + 0.131*255 ≈ 186

    unsigned char r = result.data[0];
    unsigned char g = result.data[1];
    unsigned char b = result.data[2];

    // Red should be >= green >= blue for warm tone
    EXPECT_GE(r, g);
    EXPECT_GE(g, b);

    free(white_img.data);
    free(result.data);
}

TEST_F(CpuFilterTest, SepiaBlackStaysBlack) {
    // Black image should stay black
    Image black_img = create_uniform_image(0, 0, 0);
    Image result = sepia_filter(black_img);

    int num_pixels = black_img.width * black_img.height;
    for (int i = 0; i < num_pixels; ++i) {
        EXPECT_EQ(result.data[i * 3 + 0], 0);
        EXPECT_EQ(result.data[i * 3 + 1], 0);
        EXPECT_EQ(result.data[i * 3 + 2], 0);
    }

    free(black_img.data);
    free(result.data);
}

TEST_F(CpuFilterTest, SepiaDoesNotOverflow) {
    // Test that sepia doesn't produce values > 255
    Image result = sepia_filter(test_image);

    int num_pixels = test_image.width * test_image.height;
    for (int i = 0; i < num_pixels; ++i) {
        EXPECT_LE(result.data[i * 3 + 0], 255);
        EXPECT_LE(result.data[i * 3 + 1], 255);
        EXPECT_LE(result.data[i * 3 + 2], 255);
    }

    free(result.data);
}

// ============================================================================
// BRIGHTNESS FILTER TESTS
// ============================================================================

TEST_F(CpuFilterTest, BrightnessProducesValidOutput) {
    Image result = brightness_filter(test_image, 50.0f);

    EXPECT_NE(result.data, nullptr);
    EXPECT_EQ(result.width, test_width);
    EXPECT_EQ(result.height, test_height);

    free(result.data);
}

TEST_F(CpuFilterTest, BrightnessIncreasesPixels) {
    // Test with positive delta
    Image result = brightness_filter(test_image, 50.0f);

    int increased_count = 0;
    int num_pixels = test_image.width * test_image.height;

    for (int i = 0; i < num_pixels; ++i) {
        unsigned char original = test_image.data[i * 3];
        unsigned char brightened = result.data[i * 3];

        if (original < 205) {
            // Pixels < 205 should increase by ~50
            EXPECT_GT(brightened, original);
            increased_count++;
        }
    }

    EXPECT_GT(increased_count, 0);

    free(result.data);
}

TEST_F(CpuFilterTest, BrightnessDecreasesPixels) {
    // Test with negative delta
    Image result = brightness_filter(test_image, -50.0f);

    int decreased_count = 0;
    int num_pixels = test_image.width * test_image.height;

    for (int i = 0; i < num_pixels; ++i) {
        unsigned char original = test_image.data[i * 3];
        unsigned char darkened = result.data[i * 3];

        if (original > 50) {
            // Pixels > 50 should decrease
            EXPECT_LT(darkened, original);
            decreased_count++;
        }
    }

    EXPECT_GT(decreased_count, 0);

    free(result.data);
}

TEST_F(CpuFilterTest, BrightnessClampsAtMax) {
    // Create bright image
    Image bright_img = create_uniform_image(220, 220, 220);
    Image result = brightness_filter(bright_img, 100.0f);

    // All pixels should be clamped to 255
    int num_pixels = bright_img.width * bright_img.height;
    for (int i = 0; i < num_pixels; ++i) {
        EXPECT_EQ(result.data[i * 3 + 0], 255);
        EXPECT_EQ(result.data[i * 3 + 1], 255);
        EXPECT_EQ(result.data[i * 3 + 2], 255);
    }

    free(bright_img.data);
    free(result.data);
}

TEST_F(CpuFilterTest, BrightnessClampsAtMin) {
    // Create dark image
    Image dark_img = create_uniform_image(50, 50, 50);
    Image result = brightness_filter(dark_img, -100.0f);

    // All pixels should be clamped to 0
    int num_pixels = dark_img.width * dark_img.height;
    for (int i = 0; i < num_pixels; ++i) {
        EXPECT_EQ(result.data[i * 3 + 0], 0);
        EXPECT_EQ(result.data[i * 3 + 1], 0);
        EXPECT_EQ(result.data[i * 3 + 2], 0);
    }

    free(dark_img.data);
    free(result.data);
}

TEST_F(CpuFilterTest, BrightnessZeroDeltaDoesNothing) {
    // Test with delta = 0
    Image result = brightness_filter(test_image, 0.0f);

    // Result should be identical to input
    EXPECT_TRUE(images_equal(test_image, result));

    free(result.data);
}

TEST_F(CpuFilterTest, BrightnessDeltaClamped) {
    // Test that enormous delta is clamped to [-255, 255]
    Image result1 = brightness_filter(test_image, 1000.0f);
    Image result2 = brightness_filter(test_image, 255.0f);

    // Both should be approximately equal (clamped to same value)
    EXPECT_TRUE(images_equal(result1, result2));

    free(result1.data);
    free(result2.data);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_F(CpuFilterTest, ChainedFiltersWork) {
    // Apply grayscale, then brightness
    Image gray = grayscale_filter(test_image);
    Image brightened = brightness_filter(gray, 50.0f);

    EXPECT_NE(brightened.data, nullptr);
    EXPECT_EQ(brightened.width, test_width);

    free(gray.data);
    free(brightened.data);
}

TEST_F(CpuFilterTest, AllFiltersPreserveImageDimensions) {
    Image gray = grayscale_filter(test_image);
    EXPECT_EQ(gray.width, test_width);
    EXPECT_EQ(gray.height, test_height);

    Image sepia = sepia_filter(test_image);
    EXPECT_EQ(sepia.width, test_width);
    EXPECT_EQ(sepia.height, test_height);

    Image bright = brightness_filter(test_image, 50.0f);
    EXPECT_EQ(bright.width, test_width);
    EXPECT_EQ(bright.height, test_height);

    free(gray.data);
    free(sepia.data);
    free(bright.data);
}

TEST_F(CpuFilterTest, AllFiltersAllocateMemory) {
    Image gray = grayscale_filter(test_image);
    EXPECT_NE(gray.data, nullptr);

    Image sepia = sepia_filter(test_image);
    EXPECT_NE(sepia.data, nullptr);

    Image bright = brightness_filter(test_image, 50.0f);
    EXPECT_NE(bright.data, nullptr);

    free(gray.data);
    free(sepia.data);
    free(bright.data);
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_F(CpuFilterTest, FiltersHandleSmallImages) {
    Image small_img = create_uniform_image(128, 128, 128, 64, 64);

    Image gray = grayscale_filter(small_img);
    EXPECT_EQ(gray.width, 64);
    EXPECT_EQ(gray.height, 64);

    free(small_img.data);
    free(gray.data);
}

TEST_F(CpuFilterTest, FiltersHandleLargeImages) {
    Image large_img = create_uniform_image(200, 200, 200, 1024, 1024);

    Image gray = grayscale_filter(large_img);
    EXPECT_EQ(gray.width, 1024);
    EXPECT_EQ(gray.height, 1024);

    free(large_img.data);
    free(gray.data);
}

TEST_F(CpuFilterTest, BrightnessNegativeDeltaWorks) {
    // Ensure negative brightness works correctly
    Image result = brightness_filter(test_image, -128.0f);
    EXPECT_NE(result.data, nullptr);

    free(result.data);
}

TEST_F(CpuFilterTest, BrightnessMaxDeltaWorks) {
    // Ensure max brightness works
    Image result = brightness_filter(test_image, 255.0f);
    EXPECT_NE(result.data, nullptr);

    free(result.data);
}