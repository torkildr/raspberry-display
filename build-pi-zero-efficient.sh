#!/bin/bash
# Resource-efficient build script for Raspberry Pi Zero

set -e

BUILD_TYPE="${1:-release}"

echo "Setting up resource-efficient build for Pi Zero..."

# Create swap file if it doesn't exist (helps with memory pressure)
if [ ! -f /swapfile ]; then
    echo "Creating swap file to help with compilation..."
    sudo fallocate -l 1G /swapfile
    sudo chmod 600 /swapfile
    sudo mkswap /swapfile
    sudo swapon /swapfile
    echo "Swap file created and activated"
fi

# Set up build directory with memory-efficient options
BUILD_DIR="build-pi-zero"
mkdir -p "$BUILD_DIR"

echo "Configuring meson with Pi Zero optimizations..."
meson setup "$BUILD_DIR" \
    -Dbuildtype="$BUILD_TYPE" \
    --backend=ninja \
    --prefix=/usr

# Build with limited parallelism to avoid overwhelming the Pi Zero
echo "Building with single-threaded compilation..."
cd "$BUILD_DIR"

# Use ninja with limited jobs (Pi Zero has only 1 core anyway)
ninja -j1 -v

echo "Build complete! Binaries are in $BUILD_DIR/"
ls -la

echo "To install:"
echo "  sudo ninja install"
