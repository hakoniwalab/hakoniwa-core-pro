#!/bin/bash

# This script uninstalls Hakoniwa.
# It removes the directories created by install.bash.

# Function to ask for user confirmation
confirm() {
    # call with a prompt string or use a default
    read -r -p "${1:-Are you sure? [y/N]} " response
    case "$response" in
        [yY][eE][sS]|[yY])
            true
            ;;
        *)
            false
            ;;
    esac
}


OS_TYPE="posix"
OS=`uname`
INSTALL_DIR=""
HAKONIWA_DIR=""
VAR_DIR=""

if [ "$OS" = "Linux" ] || [ "$OS" = "Darwin" ]; then
    SUDO=
    which sudo > /dev/null
    if [ $? -eq 0 ]; then
        SUDO=sudo
    fi
    INSTALL_DIR="/usr/local"
    HAKONIWA_DIR="/etc/hakoniwa"
    VAR_DIR="/var/lib/hakoniwa"
else
    OS_TYPE="win"
    SUDO=
    INSTALL_DIR="../local"
    HAKONIWA_DIR="../hakoniwa"
fi

echo "This script will attempt to remove the following Hakoniwa directories:"
echo "  - ${INSTALL_DIR}/bin/hakoniwa"
echo "  - ${INSTALL_DIR}/lib/hakoniwa"
echo "  - ${INSTALL_DIR}/include/hakoniwa"
echo "  - ${HAKONIWA_DIR}"
if [ -n "$VAR_DIR" ]; then
    echo "  - ${VAR_DIR}"
fi
echo ""
echo "Note: If a directory does not exist, it will be skipped."
echo ""

if ! confirm "Do you want to proceed with uninstallation? [y/N]"; then
    echo "Uninstallation cancelled."
    exit 1
fi

echo "Uninstalling Hakoniwa..."

if [ -d "${INSTALL_DIR}/bin/hakoniwa" ]; then
    ${SUDO} rm -rf "${INSTALL_DIR}/bin/hakoniwa"
fi
if [ -d "${INSTALL_DIR}/lib/hakoniwa" ]; then
    ${SUDO} rm -rf "${INSTALL_DIR}/lib/hakoniwa"
fi
if [ -d "${INSTALL_DIR}/include/hakoniwa" ]; then
    ${SUDO} rm -rf "${INSTALL_DIR}/include/hakoniwa"
fi
if [ -d "${HAKONIWA_DIR}" ]; then
    ${SUDO} rm -rf "${HAKONIWA_DIR}"
fi

if [ -n "$VAR_DIR" ] && [ -d "$VAR_DIR" ]; then
    ${SUDO} rm -rf "${VAR_DIR}"
fi

echo "Hakoniwa uninstallation process finished."
