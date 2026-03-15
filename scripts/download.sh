#!/bin/bash

set -e  # Exit on error

echo "=================================================="
echo "Image Downloader"
echo "=================================================="

# Get the project root directory (where this script is located)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Change to project root
cd "$PROJECT_ROOT"

# Option 1) Random images from picsum.photos
download_random() {
    echo ""
    echo "📥 Downloading 100 random images from picsum.photos..."
    
    # Create temp directory for downloads
    TEMP_DIR=$(mktemp -d)
    trap 'rm -rf $TEMP_DIR' EXIT
    
    cd "$TEMP_DIR"
    
    # Download 100 random RGB images (200x200 pixels)
    for i in {1..100}; do
        echo " - image $i/100"
        curl -s -L "https://picsum.photos/200/200?random=$i" -o "image$i.jpg"
    done
    
    # Move to final destination
    cd "$PROJECT_ROOT"
    mkdir -p images/random
    mv "$TEMP_DIR"/* images/random/
    
    echo "Download complete: images saved to images/random/"
    # ls -lh images/random/ | head -11  # Show first 10 images + header
    # echo "  ... ($(ls images/random/ | wc -l) images total)"
}

# Option 2) Google Flowers dataset
download_flowers() {
    echo ""
    echo "📥 Downloading Google Flowers dataset..."
    
    # Create temp directory for downloads
    TEMP_DIR=$(mktemp -d)
    trap 'rm -rf $TEMP_DIR' EXIT
    
    cd "$TEMP_DIR"
    
    # Download the compressed tarball (approx. 218MB)
    echo " - Downloading archive (this may take some time)..."
    curl -s -O http://download.tensorflow.org/example_images/flower_photos.tgz
    
    # Extract the files
    echo " - Extracting files..."
    tar -xzf flower_photos.tgz
    
    # Move to final destination
    cd "$PROJECT_ROOT"
    mkdir -p images/flowers
    find "$TEMP_DIR/flower_photos" -name "*.jpg" | head -n 100 | xargs -I {} cp {} images/flowers/

    # Change images name to flower<n>.jpg:
    cd images/flowers
    i=1
    for img in *.jpg; do
        mv "$img" "flower$i.jpg"
        i=$((i + 1))
    done
    
    echo "Download complete: flowers saved to images/flowers/"
    # ls -lh images/flowers/ | head -11  # Show first 10 images + header
    # echo "  ... ($(ls images/flowers/ | wc -l) images total)"
}

# Main execution
echo ""
echo "Choose download dataset:"
echo "  1) Random images - picsum.photos (100 images)"
echo "  2) Google Flowers - tensorflow.org (100 images)"
echo ""
read -r -p "Enter choice (1-2): " choice

case $choice in
    1)
        download_random
        ;;
    2)
        download_flowers
        ;;
    *)
        echo "Invalid choice. Exiting..."
        exit 1
        ;;
esac
