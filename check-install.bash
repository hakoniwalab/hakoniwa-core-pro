#!/bin/bash
# This script checks if all files from the hakoniwa-core installation exist.
# It uses the 'install_manifest.txt' created during the installation process.

set -e

# Define the build directory.
BUILD_DIR="cmake-build"
MANIFEST_FILE="${BUILD_DIR}/install_manifest.txt"

# A flag to track if any files are missing.
ALL_FILES_FOUND=true


echo "Checking Hakoniwa installation status..."

# Check if the install manifest exists.
if [ ! -f "${MANIFEST_FILE}" ]; then
    echo "Error: Installation manifest not found at '${MANIFEST_FILE}'" >&2
    echo "Please run the installation first, or ensure you are in the correct project root directory." >&2
    exit 1
fi

echo "Reading file list from ${MANIFEST_FILE}..."

# Read the manifest file line by line and check if each file exists.
while IFS= read -r file_path || [ -n "$file_path" ]; do
    if [ -e "$file_path" ]; then
        # Use printf for consistent output without newline issues.
        printf "  [ EXISTS ] %s\n" "$file_path"
    else
        printf "  [ MISSING] %s\n" "$file_path"
        ALL_FILES_FOUND=false
    fi
done < "${MANIFEST_FILE}"

if [ ! -d /var/lib/hakoniwa/mmap ]
then
    echo "  [ MISSING] /var/lib/hakoniwa/mmap"
    ALL_FILES_FOUND=false
else
    echo "  [ EXISTS ] /var/lib/hakoniwa/mmap"
fi


echo ""
# Final result.
if [ "$ALL_FILES_FOUND" = true ]; then
    echo "Success: All installed files were found."
    exit 0
else
    echo "Error: Some installed files are missing. Please check the log above." >&2
    exit 1
fi