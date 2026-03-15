#!/bin/bash

set -e  # Exit on error
set +x # Disable command echo for cleaner output

# Assert to be in project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the project root directory."
    exit 1
fi

# Check dependencies in the lib directory
echo "Checking dependencies..."

# Check STB headers
if [ ! -f "lib/stb/stb_image.h" ] || [ ! -f "lib/stb/stb_image_write.h" ]; then
    echo "⬇️  Downloading STB Image libraries..."
    mkdir -p lib/stb
    curl -o lib/stb/stb_image.h \
        https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    curl -o lib/stb/stb_image_write.h \
        https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
    echo "✅ STB libraries downloaded"
else
    echo "✅ STB libraries found"
fi

# Check spdlog
if [ ! -d "lib/spdlog/spdlog-1.14.1" ]; then
    echo "⬇️  Downloading spdlog..."
    mkdir -p lib/spdlog
    cd lib/spdlog
    curl -L https://github.com/gabime/spdlog/archive/refs/tags/v1.14.1.zip -o spdlog.zip
    unzip -q spdlog.zip
    rm spdlog.zip
    cd ../..
    echo "✅ spdlog downloaded"
else
    echo "✅ spdlog found"
fi
echo ""

# Run cmake compilation process
mkdir -p build
cd build || exit
cmake ..
PROCS=$(($(nproc) - 2))
make -j "$PROCS"
cd .. || exit

