#!/bin/bash
# This script builds and installs hakoniwa-core for Unix-like systems (Linux, macOS).

set -e
BUILD_DIR="cmake-build"
INSTALL_PREFIX="/usr/local/hakoniwa"  # Default installation prefix
ASSET_NUM=4  # Default asset number

echo "Starting Hakoniwa build..."

echo "Step 1/2: Building project with CMake..."
cmake -B ${BUILD_DIR} -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} $ENABLE_HAKO_TIME_MEASURE_FLAG -DHAKO_DATA_MAX_ASSET_NUM=${ASSET_NUM} $BUILD_C_FLAGS
cmake --build ${BUILD_DIR}

# 2. Install the project.
#    This command requires administrator privileges (sudo).
#    It copies files to the system directories based on CMAKE_INSTALL_PREFIX.
#    It also creates 'install_manifest.txt' in the build directory.
echo "Step 2/2: Installing project to ${INSTALL_PREFIX}..."
sudo cmake --install ${BUILD_DIR}

echo "Configuring directory for mmap files..."
sudo mkdir -p /var/lib/hakoniwa/mmap/
sudo chmod 755 /var/lib/hakoniwa
sudo chmod 777 /var/lib/hakoniwa/mmap/


echo ""
echo "Hakoniwa installation completed successfully."
echo "Installation manifest is located at: ${BUILD_DIR}/install_manifest.txt"
