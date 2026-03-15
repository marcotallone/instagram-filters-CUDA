#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "config.h"
#include "imageIO.h"
#include "logger.h"

namespace fs = std::filesystem;

// Test Fixture
// (A fixture is a reusable setup/teardown context for related tests.)
class ImageIOTest : public ::testing::Test {
protected:
    // Temporary test folder variable
    fs::path test_dir;

    // Run BEFORE each test: create a temporary folder
    void SetUp() override {
        init_logger();
        test_dir = fs::temp_directory_path() / "test_images";
        if (!fs::exists(test_dir)) { fs::create_directories(test_dir); }
    }

    // Run AFTER each test: destroy temporary environment folders
    void TearDown() override { fs::remove_all(test_dir); }
};

TEST_F(ImageIOTest, LoadsPNGImage) {
    // Copy a test PNG image from images/input to temp dir
    fs::path source = config::IMAGES_PATH / "input" / "Lena.png";
    fs::path destination = test_dir / "Lena.png";

    // Verify the source exists
    if (!fs::exists(source)) {
        FAIL() << "Test image not found at: " << source;
    }

    fs::copy(source, destination);
    auto images = load_images(test_dir);

    EXPECT_EQ(images.size(), 1);
    EXPECT_EQ(images[0].filename, "Lena.png");
    EXPECT_GT(images[0].width, 0);
    EXPECT_GT(images[0].height, 0);
    EXPECT_EQ(images[0].channels, 3);
    EXPECT_EQ(images[0].extension, ".png");

    free_image(images[0]);
}

TEST_F(ImageIOTest, LoadsJPGImage) {
    // Copy a test JPG image from images/input to temp dir
    fs::path source = config::IMAGES_PATH / "input" / "mountain.jpg";
    fs::path destination = test_dir / "mountain.jpg";

    // Verify the source exists
    if (!fs::exists(source)) {
        FAIL() << "Test image not found at: " << source;
    }

    fs::copy(source, destination);
    auto images = load_images(test_dir);

    EXPECT_EQ(images.size(), 1);
    EXPECT_EQ(images[0].filename, "mountain.jpg");
    EXPECT_GT(images[0].width, 0);
    EXPECT_GT(images[0].height, 0);
    EXPECT_EQ(images[0].channels, 3);
    EXPECT_EQ(images[0].extension, ".jpg");

    free_image(images[0]);
}

TEST_F(ImageIOTest, ThrowsOnMissingDirectory) {
    EXPECT_THROW(load_images(test_dir / "nonexistent"), std::runtime_error);
}

TEST_F(ImageIOTest, IgnoresUnsupportedFormats) {
    // Create dummy data just to test correct handling of unsupported extensions
    std::vector<std::string> unsupported = {"test.txt", "image.bmp",
                                            "data.json"};
    for (const auto& fname : unsupported) {
        std::ofstream dummy(test_dir / fname);
        dummy << "garbage data";
        dummy.close();
    }

    auto images = load_images(test_dir);
    EXPECT_EQ(images.size(), 0);

    // TearDown() runs here → fs::remove_all(test_dir) deletes the temp folder
}
