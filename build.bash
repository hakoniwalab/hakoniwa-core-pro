#!/bin/bash


# C_FLAGS と CXX_FLAGS に -m32 オプションを追加
# BUILD_C_FLAGS="-DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULTS_FILE="${SCRIPT_DIR}/cmake/hako_build_defaults.conf"

if [ ! -f "${DEFAULTS_FILE}" ]; then
    echo "ERROR: build defaults file not found: ${DEFAULTS_FILE}" >&2
    exit 1
fi

# shellcheck disable=SC1090
source "${DEFAULTS_FILE}"

DEFAULT_HAKO_ASSET_NUM=${HAKO_DATA_MAX_ASSET_NUM}
if [ -n "${ASSET_NUM:-}" ] && [ "${ASSET_NUM}" -gt "${DEFAULT_HAKO_ASSET_NUM}" ]; then
    :
else
    ASSET_NUM=${DEFAULT_HAKO_ASSET_NUM}
fi
DEFAULT_HAKO_SERVICE_MAX=${HAKO_SERVICE_MAX}
if [ -n "${SERVICE_MAX:-}" ] && [ "${SERVICE_MAX}" -gt 0 ]; then
    :
else
    SERVICE_MAX=${DEFAULT_HAKO_SERVICE_MAX}
fi
DEFAULT_HAKO_RECV_EVENT_MAX=${HAKO_RECV_EVENT_MAX}
if [ -n "${RECV_EVENT_MAX:-}" ] && [ "${RECV_EVENT_MAX}" -gt 0 ]; then
    :
else
    RECV_EVENT_MAX=${DEFAULT_HAKO_RECV_EVENT_MAX}
fi
DEFAULT_HAKO_SERVICE_CLIENT_MAX=${HAKO_SERVICE_CLIENT_MAX}
if [ -n "${SERVICE_CLIENT_MAX:-}" ] && [ "${SERVICE_CLIENT_MAX}" -gt 0 ]; then
    :
else
    SERVICE_CLIENT_MAX=${DEFAULT_HAKO_SERVICE_CLIENT_MAX}
fi
DEFAULT_HAKO_PDU_CHANNEL_MAX=${HAKO_PDU_CHANNEL_MAX}
if [ -n "${CHANNEL_MAX:-}" ] && [ "${CHANNEL_MAX}" -gt 0 ]; then
    :
else
    CHANNEL_MAX=${DEFAULT_HAKO_PDU_CHANNEL_MAX}
fi
echo "ASSET_NUM is ${ASSET_NUM}"
echo "SERVICE_MAX is ${SERVICE_MAX}"
echo "RECV_EVENT_MAX is ${RECV_EVENT_MAX}"
echo "SERVICE_CLIENT_MAX is ${SERVICE_CLIENT_MAX}"
echo "CHANNEL_MAX is ${CHANNEL_MAX}"

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
        cmake .. -DCMAKE_INSTALL_PREFIX=/usr $ENABLE_HAKO_TIME_MEASURE_FLAG \
            -DHAKO_DATA_MAX_ASSET_NUM=${ASSET_NUM} \
            -DHAKO_SERVICE_MAX=${SERVICE_MAX} \
            -DHAKO_RECV_EVENT_MAX=${RECV_EVENT_MAX} \
            -DHAKO_SERVICE_CLIENT_MAX=${SERVICE_CLIENT_MAX} \
            -DHAKO_PDU_CHANNEL_MAX=${CHANNEL_MAX} \
            $BUILD_C_FLAGS
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
