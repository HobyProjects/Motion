#!/bin/bash
# Setup script for Motion Engine Version Control System

echo "Setting up Version Control System for Motion Engine..."

# Check if we're in the project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from your project root directory"
    exit 1
fi

# Create cmake directory if it doesn't exist
if [ ! -d "cmake" ]; then
    echo "Creating cmake directory..."
    mkdir cmake
fi

# Copy files (assuming they're in the same directory as this script)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Copying GenerateVersionHeader.cmake..."
cp "$SCRIPT_DIR/cmake/GenerateVersionHeader.cmake" cmake/

echo "Copying Version.h.in..."
cp "$SCRIPT_DIR/config/MotionVersion.hpp.in" cmake/

echo "Backing up original CMakeLists.txt..."
cp CMakeLists.txt CMakeLists.txt.backup

echo "Updating CMakeLists.txt..."
cp "$SCRIPT_DIR/CMakeLists.txt" ./

echo ""
echo "✓ Version control system installed successfully!"
echo ""
echo "Next steps:"
echo "1. Review the changes to CMakeLists.txt"
echo "2. Reconfigure your build: cmake -B build"
echo "3. Build your project: cmake --build build"
echo "4. Include 'MotionVersion.hpp' in your code to access version info"
echo ""
echo "See VERSION_CONTROL_README.md for detailed usage instructions."
echo ""
echo "Your original CMakeLists.txt has been backed up to CMakeLists.txt.backup"