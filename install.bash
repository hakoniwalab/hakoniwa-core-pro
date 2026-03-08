#!/bin/bash
# This script builds and installs hakoniwa-core for Unix-like systems (Linux, macOS).

set -e
BUILD_DIR="cmake-build"
INSTALL_PREFIX="/usr/local/hakoniwa"  # Default installation prefix
DEFAULT_HAKO_ASSET_NUM=16
if [ -n "${ASSET_NUM:-}" ] && [ "${ASSET_NUM}" -gt "${DEFAULT_HAKO_ASSET_NUM}" ]; then
    :
else
    ASSET_NUM=${DEFAULT_HAKO_ASSET_NUM}
fi
DEFAULT_HAKO_SERVICE_MAX=4096
if [ -n "${SERVICE_MAX:-}" ] && [ "${SERVICE_MAX}" -gt 0 ]; then
    :
else
    SERVICE_MAX=${DEFAULT_HAKO_SERVICE_MAX}
fi
DEFAULT_HAKO_RECV_EVENT_MAX=16384
if [ -n "${RECV_EVENT_MAX:-}" ] && [ "${RECV_EVENT_MAX}" -gt 0 ]; then
    :
else
    RECV_EVENT_MAX=${DEFAULT_HAKO_RECV_EVENT_MAX}
fi
DEFAULT_HAKO_SERVICE_CLIENT_MAX=1024
if [ -n "${SERVICE_CLIENT_MAX:-}" ] && [ "${SERVICE_CLIENT_MAX}" -gt 0 ]; then
    :
else
    SERVICE_CLIENT_MAX=${DEFAULT_HAKO_SERVICE_CLIENT_MAX}
fi
echo "ASSET_NUM is ${ASSET_NUM}"
echo "SERVICE_MAX is ${SERVICE_MAX}"
echo "RECV_EVENT_MAX is ${RECV_EVENT_MAX}"
echo "SERVICE_CLIENT_MAX is ${SERVICE_CLIENT_MAX}"

echo "Starting Hakoniwa build..."

echo "Step 1/2: Building project with CMake..."
cmake -B ${BUILD_DIR} -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} $ENABLE_HAKO_TIME_MEASURE_FLAG \
  -DHAKO_DATA_MAX_ASSET_NUM=${ASSET_NUM} \
  -DHAKO_SERVICE_MAX=${SERVICE_MAX} \
  -DHAKO_RECV_EVENT_MAX=${RECV_EVENT_MAX} \
  -DHAKO_SERVICE_CLIENT_MAX=${SERVICE_CLIENT_MAX} \
  $BUILD_C_FLAGS
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
