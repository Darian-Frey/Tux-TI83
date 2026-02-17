#!/bin/bash

# Tux-TI83 Automated Build Script
# Protocol: SCHEMA_V5

PROJECT_DIR=$(pwd)
BUILD_DIR="$PROJECT_DIR/build"

echo "-----------------------------------------------"
echo "Initializing Tux-TI83 Build..."
echo "-----------------------------------------------"

# 1. Check for dependencies
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is not installed. Install with: sudo apt install cmake"
    exit 1
fi

# 2. Create build directory if it doesn't exist (Fixed hyphen)
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

# 3. Enter build directory and run CMake
cd "$BUILD_DIR" || exit
echo "Running CMake configuration..."
cmake ..

# 4. Compile the project
echo "Compiling..."
make -j$(nproc)

# 5. Check if build was successful
if [ -f "tux_ti83" ]; then
    echo "-----------------------------------------------"
    echo "Build Successful!"
    echo "Launching Tux-TI83..."
    echo "-----------------------------------------------"
    ./tux_ti83
else
    echo "Build failed. Please check the error messages above."
    exit 1
fi
