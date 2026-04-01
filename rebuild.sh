#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "--- Starting Firmware Rebuild ---"

# 1. Enter the firmware directory
cd firmware

# 2. Clean and recreate the build directory for a fresh build
if [ -d "build" ]; then
    echo "Cleaning up old build directory..."
    rm -rf build
fi

mkdir build
cd build

# 3. Configure the build system with CMake
echo "Configuring with CMake..."
cmake ..

# 4. Compile using all available processor cores
echo "Compiling..."
make -j$(nproc)

echo "--- Build Successful! ---"

# 5. Return to the project root to run Renode
cd ../..

# 6. Execute the Renode simulator
# Ensure 'renode' is in your system PATH
echo "Launching Renode Simulation..."
renode platform/run.resc