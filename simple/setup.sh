#!/bin/bash

# This script helps set up and install all dependencies needed for the SimpleSegmentation project
# It's designed for Ubuntu/Debian systems

set -e  # Exit on any error

echo "===== Installing dependencies for SimpleSegmentation ====="

# Update package lists
echo "[1/5] Updating package lists..."
sudo apt-get update

# Install build essential tools
echo "[2/5] Installing build tools..."
sudo apt-get install -y build-essential cmake pkg-config

# Install OpenCV dependencies
echo "[3/5] Installing OpenCV and its dependencies..."
sudo apt-get install -y libopencv-dev

# Install curl and SSL libraries
echo "[4/5] Installing curl and SSL libraries..."
sudo apt-get install -y libcurl4-openssl-dev libssl-dev

# Create build directory and build the project
echo "[5/5] Setting up build environment..."
mkdir -p build
cd build

echo "===== Configuring project ====="
cmake ..

echo "===== Building project ====="
make -j$(nproc)

echo "===== Build complete ====="
echo ""
echo "To run the segmentation client:"
echo "  ./segmentation_client [image_path] [server_url]"
echo ""
echo "Example:"
echo "  ./segmentation_client ../test_image.jpg http://localhost:8000/segment"
echo ""
echo "Note: Make sure your segmentation server is running before testing!"