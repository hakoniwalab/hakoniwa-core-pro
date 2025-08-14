#!/bin/bash
# This script uninstalls hakoniwa-core for Unix-like systems (Linux, macOS).
# It uses the 'install_manifest.txt' created during the installation process.

set -e

# Define the build directory.
BUILD_DIR="cmake-build"
MANIFEST_FILE="${BUILD_DIR}/install_manifest.txt"

echo "Starting Hakoniwa uninstallation..."

# Check if the install manifest exists.
if [ ! -f "${MANIFEST_FILE}" ]; then
    echo "Error: Installation manifest not found at '${MANIFEST_FILE}'" >&2
    echo "Please run the installation first, or ensure you are in the correct project root directory." >&2
    exit 1
fi

echo "Uninstalling files listed in ${MANIFEST_FILE}..."

# Read the manifest and remove the files.
# This requires administrator privileges (sudo).
# xargs reads the file list and passes them to 'sudo rm'.
if xargs sudo rm < "${MANIFEST_FILE}"; then
    echo "Successfully removed all files."
else
    echo "An error occurred during file removal." >&2
    exit 1
fi

# Optional: You might want to remove the now-empty directories.
# For simplicity, this script only removes files listed in the manifest.
# CMake-installed directories will be left behind but should be empty.

echo ""
echo "Hakoniwa uninstallation completed successfully."