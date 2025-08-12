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

OS_TYPE="posix"
OS=`uname`
if [ "$OS" = "Linux" -o "$OS" = "Darwin"  ]
then
	:
else
    OS_TYPE="win"
fi

if [ $# -eq 0 ]
then
    cd cmake-build
    if [ ${OS_TYPE} = "posix" ]
    then
        cmake .. $ENABLE_HAKO_TIME_MEASURE_FLAG -DHAKO_DATA_MAX_ASSET_NUM=${ASSET_NUM} $BUILD_C_FLAGS
        make
    else
        cmake ..
        cmake --build . --target ALL_BUILD --config Release
    fi
else
	rm -rf ./cmake-build/*
fi

