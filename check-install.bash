#!/bin/bash

echo "Checking Hakoniwa installation status..."

# --- Configuration ---
OS=`uname`
INSTALL_DIR=""
HAKONIWA_DIR=""
VAR_DIR=""
LIBCONDUCTOR_BASE="libconductor"
LIBSHAKOC_BASE="libshakoc"
HAKOCMD_BASE="hako-cmd"

if [ "$OS" = "Linux" ]; then
    INSTALL_DIR="/usr/local"
    HAKONIWA_DIR="/etc/hakoniwa"
    VAR_DIR="/var/lib/hakoniwa"
    LIBCONDUCTOR_EXT="so"
    LIBSHAKOC_EXT="so"
    HAKOCMD_EXT=""
elif [ "$OS" = "Darwin" ]; then
    INSTALL_DIR="/usr/local"
    HAKONIWA_DIR="/etc/hakoniwa"
    VAR_DIR="/var/lib/hakoniwa"
    LIBCONDUCTOR_EXT="dylib"
    LIBSHAKOC_EXT="dylib"
    HAKOCMD_EXT=""
else # Assuming Windows-like
    INSTALL_DIR="../local"
    HAKONIWA_DIR="../hakoniwa"
    # No VAR_DIR on windows according to install.bash
    LIBCONDUCTOR_BASE="conductor"
    LIBSHAKOC_BASE="shakoc"
    LIBCONDUCTOR_EXT="dll"
    LIBSHAKOC_EXT="dll"
    HAKOCMD_EXT=".exe"
fi

# --- File & Directory Lists ---
DIRECTORIES=(
    "${INSTALL_DIR}/bin/hakoniwa"
    "${INSTALL_DIR}/lib/hakoniwa"
    "${INSTALL_DIR}/include/hakoniwa"
    "${HAKONIWA_DIR}"
)
# Add OS-specific directories
if [ -n "$VAR_DIR" ]; then
    DIRECTORIES+=("${VAR_DIR}")
fi


FILES=(
    "${INSTALL_DIR}/bin/hakoniwa/${HAKOCMD_BASE}${HAKOCMD_EXT}"
    "${INSTALL_DIR}/lib/hakoniwa/${LIBCONDUCTOR_BASE}.${LIBCONDUCTOR_EXT}"
    "${INSTALL_DIR}/lib/hakoniwa/${LIBSHAKOC_BASE}.${LIBSHAKOC_EXT}"
    "${INSTALL_DIR}/include/hakoniwa/hako_asset.h"
    "${HAKONIWA_DIR}/cpp_core_config.json"
)

# --- Check Logic ---
missing_items=()
found_items=0

echo ""

# Check directories
for dir in "${DIRECTORIES[@]}"; do
    if [ -d "$dir" ]; then
        echo "[ FOUND ] Directory: $dir"
        ((found_items++))
    else
        echo "[ MISSING ] Directory: $dir"
        missing_items+=("$dir")
    fi
done

# Check files
for file in "${FILES[@]}"; do
    # Use a wildcard for libassets since its name can vary
    if [[ "$file" == *"libassets"* ]]; then
        if ls "${INSTALL_DIR}/lib/hakoniwa/libassets."* 1> /dev/null 2>&1; then
            echo "[ FOUND ] File (wildcard): ${INSTALL_DIR}/lib/hakoniwa/libassets.*"
            ((found_items++))
        else
            echo "[ MISSING ] File (wildcard): ${INSTALL_DIR}/lib/hakoniwa/libassets.*"
            missing_items+=("${INSTALL_DIR}/lib/hakoniwa/libassets.*")
        fi
    else
        if [ -f "$file" ]; then
            echo "[ FOUND ] File: $file"
            ((found_items++))
        else
            echo "[ MISSING ] File: $file"
            missing_items+=("$file")
        fi
    fi
done

# --- Reporting ---
echo ""
total_items=$((${#DIRECTORIES[@]} + ${#FILES[@]}))

if [ ${#missing_items[@]} -eq 0 ]; then
    echo -e "\033[32m✅ All ${total_items} items found. Hakoniwa appears to be installed correctly.\033[0m"
    exit 0
else
    echo -e "\033[31m❌ Found ${found_items} of ${total_items} items.\033[0m"
    echo "Hakoniwa installation seems incomplete or missing."
    echo "Missing items:"
    for item in "${missing_items[@]}"; do
        echo "  - $item"
    done
    exit 1
fi
