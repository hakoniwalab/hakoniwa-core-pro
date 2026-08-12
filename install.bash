#!/bin/bash
# Build (when needed) and install hakoniwa-core on Linux/macOS.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${HAKO_BUILD_DIR:-${BUILD_DIR:-${SCRIPT_DIR}/cmake-build}}"
INSTALL_PREFIX="${HAKO_INSTALL_PREFIX:-${INSTALL_PREFIX:-/usr/local/hakoniwa}}"
MMAP_DIR="${HAKO_CORE_MMAP_PATH:-/var/lib/hakoniwa/mmap}"
HAKO_INSTALL_USE_SUDO="${HAKO_INSTALL_USE_SUDO:-ON}"

run_privileged() {
    if [ "${HAKO_INSTALL_USE_SUDO}" = "OFF" ]; then
        "$@"
    else
        sudo "$@"
    fi
}

echo "Hakoniwa build directory: ${BUILD_DIR}"
echo "Hakoniwa install prefix : ${INSTALL_PREFIX}"

if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    echo "Configured build tree not found; running build.bash first..."
    HAKO_BUILD_DIR="${BUILD_DIR}" \
    HAKO_INSTALL_PREFIX="${INSTALL_PREFIX}" \
        bash "${SCRIPT_DIR}/build.bash"
else
    # Preserve the complete build profile already stored in CMakeCache.txt
    # (limits, Python/SOABI, config paths and compiler options). Only update the
    # install prefix requested by this installation.
    echo "Using existing configured build tree without replacing its build profile."
    cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"
    cmake --build "${BUILD_DIR}"
fi

echo "Installing project to ${INSTALL_PREFIX}..."
run_privileged cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

echo "Configuring directory for mmap files: ${MMAP_DIR}"
run_privileged mkdir -p "${MMAP_DIR}"
run_privileged chmod 777 "${MMAP_DIR}"

echo ""
echo "Hakoniwa installation completed successfully."
echo "Installation manifest is located at: ${BUILD_DIR}/install_manifest.txt"
