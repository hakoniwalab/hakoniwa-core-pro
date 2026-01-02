#!/bin/bash


# C_FLAGS と CXX_FLAGS に -m32 オプションを追加
# BUILD_C_FLAGS="-DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32"

DEFAULT_HAKO_ASSET_NUM=4
if [ ! -z "${ASSET_NUM}" ] && [ ${ASSET_NUM} -gt ${DEFAULT_HAKO_ASSET_NUM} ]; then
    :
else
    ASSET_NUM=${DEFAULT_HAKO_ASSET_NUM}
fi
echo "ASSET_NUM is ${ASSET_NUM}"

# ----------------------------------------
# Detect OS type
# ----------------------------------------

OS_TYPE="posix"
OS=`uname`
if [ "$OS" = "Linux" -o "$OS" = "Darwin"  ]
then
    echo $OS_TYPE
else
    OS_TYPE="win"
fi

# ----------------------------------------
# Build or Clean
# ----------------------------------------

BUILD_DIR="cmake-build"

if [ $# -eq 0 ]
then
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    if [ ${OS_TYPE} = "posix" ]
    then
        cmake .. -DCMAKE_INSTALL_PREFIX=/usr $ENABLE_HAKO_TIME_MEASURE_FLAG -DHAKO_DATA_MAX_ASSET_NUM=${ASSET_NUM} $BUILD_C_FLAGS
        make
    else
        cmake ..
        cmake --build . --target ALL_BUILD --config Release
    fi
else
    # ---- Clean mode ----
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR:?}"/*
fi
