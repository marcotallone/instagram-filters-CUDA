#!/bin/bash

set -e  # Exit on error

echo "=================================================="
echo "GPU Image Filter"
echo "=================================================="
echo ""

# Get the project root directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Change to project root
cd "$PROJECT_ROOT"

# Check if executable exists
if [ ! -f "bin/instagram_filtersGPU" ]; then
    echo "❌ Error: instagram_filtersGPU executable not found in bin/"
    echo "Please build the project with CUDA enabled:"
    echo "  cd build && cmake -DENABLE_CUDA=ON -DENABLE_DEBUG=ON .. && cmake --build ."
    exit 1
fi

# Define paths
INPUT_DIR="images/input"
OUTPUT_DIR="images/output"
EXECUTABLE="./bin/instagram_filtersGPU"

# Check if input directory exists
if [ ! -d "$INPUT_DIR" ]; then
    echo "❌ Error: Input directory '$INPUT_DIR' not found"
    exit 1
fi

# Check if input has images
IMAGE_COUNT=$(find "$INPUT_DIR" -type f \( -name "*.jpg" -o -name "*.png" -o -name "*.jpeg" \) | wc -l)
if [ "$IMAGE_COUNT" -eq 0 ]; then
    echo "❌ Error: No images found in $INPUT_DIR"
    echo "Please run: ./scripts/download.sh"
    exit 1
fi

echo "Found $IMAGE_COUNT images in $INPUT_DIR"
echo ""

# Clean previous output (optional)
if [ -d "$OUTPUT_DIR" ]; then
    echo "Cleaning previous output directory..."
    rm -rf "$OUTPUT_DIR"
fi

echo "Applying grayscale filter..."
$EXECUTABLE -i "$INPUT_DIR" -o "$OUTPUT_DIR/grayscaleGPU" -f grayscale
echo ""

echo "Applying sepia filter..."
$EXECUTABLE -i "$INPUT_DIR" -o "$OUTPUT_DIR/sepiaGPU" -f sepia
echo ""

echo "Applying brightness +100 filter..."
$EXECUTABLE -i "$INPUT_DIR" -o "$OUTPUT_DIR/brightness_plusGPU" -f brightness -d 100
echo ""

echo "Applying brightness -100 filter..."
$EXECUTABLE -i "$INPUT_DIR" -o "$OUTPUT_DIR/brightness_minusGPU" -f brightness -d -100
echo ""

echo ""
echo "Filtered images can be found in:"
echo "  - images/output/grayscaleGPU/"
echo "  - images/output/sepiaGPU/"
echo "  - images/output/brightness_plusGPU/"
echo "  - images/output/brightness_minusGPU/"
echo ""