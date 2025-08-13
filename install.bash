#!/bin/bash
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

echo "This script will install Hakoniwa to the following directories:"
echo "  - Install directory: ${INSTALL_DIR}"
echo "  - Config directory:  ${HAKONIWA_DIR}"
if [ -n "$VAR_DIR" ]; then
    echo "  - Data directory:    ${VAR_DIR}"
fi
echo ""

if ! confirm "Do you want to proceed with installation? [y/N]"; then
    echo "Installation cancelled."
    exit 1
fi

echo "Installing Hakoniwa..."


${SUDO} mkdir -p ${INSTALL_DIR}/bin/hakoniwa
${SUDO} mkdir -p ${INSTALL_DIR}/lib/hakoniwa
${SUDO} mkdir -p ${INSTALL_DIR}/lib/hakoniwa/py
${SUDO} mkdir -p ${INSTALL_DIR}/include/hakoniwa
${SUDO} mkdir -p ${HAKONIWA_DIR}

if [ "$OS" = "Linux" -o "$OS" = "Darwin"  ]
then
	${SUDO} mkdir -p /var/lib/hakoniwa/mmap
fi

${SUDO} cp hakoniwa-core-cpp/cpp_core_config.json ${HAKONIWA_DIR}

if [ "$OS" = "Darwin"  ]
then
    ${SUDO} cp ./cmake-build/sources/assets/callback/libassets.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/conductor/libconductor.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/polling/libshakoc.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/bindings/python/libhakopy.dylib ${INSTALL_DIR}/lib/hakoniwa/py/hakopy.so
	${SUDO} cp ./cmake-build/sources/assets/polling/libshakoc.dylib ${INSTALL_DIR}/lib/hakoniwa/hakoc.so
    ${SUDO} cp ./cmake-build/sources/command/hako-cmd ${INSTALL_DIR}/bin/hakoniwa/
elif [ "$OS" = "Linux"  ]
then
    ${SUDO} cp ./cmake-build/sources/assets/callback/libassets.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/conductor/libconductor.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/polling/libshakoc.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/bindings/python/libhakopy.so ${INSTALL_DIR}/lib/hakoniwa/py/hakopy.so
	${SUDO} cp ./cmake-build/sources/assets/polling/libshakoc.so ${INSTALL_DIR}/lib/hakoniwa/hakoc.so
    ${SUDO} cp ./cmake-build/sources/command/hako-cmd ${INSTALL_DIR}/bin/hakoniwa/
else
    ${SUDO} cp ./cmake-build/sources/assets/callback/assets.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/conductor/conductor.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/polling/shakoc.* ${INSTALL_DIR}/lib/hakoniwa/
    ${SUDO} cp ./cmake-build/sources/assets/bindings/python/hakopy.dll ${INSTALL_DIR}/lib/hakoniwa/py/hakopy.pyd
	${SUDO} cp ./cmake-build/sources/assets/polling/shakoc.so ${INSTALL_DIR}/lib/hakoniwa/hakoc.so
    ${SUDO} cp ./cmake-build/sources/command/hako-cmd.exe ${INSTALL_DIR}/bin/hakoniwa/
fi

${SUDO} cp include/* ${INSTALL_DIR}/include/hakoniwa/

if [ -z $USER ]
then
	:
else
	# ディレクトリの所有者をインストールユーザーに変更
	${SUDO} chown -R $USER /var/lib/hakoniwa
fi

# ディレクトリのパーミッションを適切に設定
if [ "$OS" = "Linux" -o "$OS" = "Darwin"  ]
then
	${SUDO} chmod -R 755 /var/lib/hakoniwa
fi


# for temporary use
#DEST_UNITY_DIR=../../oss/hakoniwa-unity-drone/simulation/Library/PackageCache/com.hakoniwa-lab.hakoniwa-sim@f3739a3363aa/Plugins/arm64/
#cp /usr/local/lib/hakoniwa/libconductor.dylib ${DEST_UNITY_DIR}
#cp /usr/local/lib/hakoniwa/libshakoc.dylib ${DEST_UNITY_DIR}
echo "Hakoniwa installation finished."