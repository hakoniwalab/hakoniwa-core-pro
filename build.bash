#!/bin/bash


# C_FLAGS と CXX_FLAGS に -m32 オプションを追加
# BUILD_C_FLAGS="-DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULTS_FILE="${HAKO_BUILD_DEFAULTS_FILE:-${SCRIPT_DIR}/cmake/hako_build_defaults.conf}"

if [ ! -f "${DEFAULTS_FILE}" ]; then
    echo "ERROR: build defaults file not found: ${DEFAULTS_FILE}" >&2
    exit 1
fi

HAKO_DATA_MAX_ASSET_NUM=
HAKO_PDU_CHANNEL_MAX=
HAKO_RECV_EVENT_MAX=
HAKO_SERVICE_CLIENT_MAX=
HAKO_SERVICE_MAX=
HAKO_CLIENT_NAMELEN_MAX=
HAKO_SERVICE_NAMELEN_MAX=

while IFS= read -r line || [ -n "${line}" ]; do
    line="${line#"${line%%[![:space:]]*}"}"
    line="${line%"${line##*[![:space:]]}"}"
    case "${line}" in
        ""|\#*)
            continue
            ;;
    esac
    if [[ ! "${line}" =~ ^([A-Z0-9_]+)=([0-9]+)$ ]]; then
        echo "ERROR: invalid build defaults entry: ${line}" >&2
        exit 1
    fi
    key="${BASH_REMATCH[1]}"
    value="${BASH_REMATCH[2]}"
    case "${key}" in
        HAKO_DATA_MAX_ASSET_NUM|HAKO_PDU_CHANNEL_MAX|HAKO_RECV_EVENT_MAX|\
        HAKO_SERVICE_CLIENT_MAX|HAKO_SERVICE_MAX|HAKO_CLIENT_NAMELEN_MAX|\
        HAKO_SERVICE_NAMELEN_MAX)
            ;;
        *)
            echo "ERROR: unknown build defaults key: ${key}" >&2
            exit 1
            ;;
    esac
    if [ "${value}" -le 0 ]; then
        echo "ERROR: build default must be a positive integer: ${key}" >&2
        exit 1
    fi
    printf -v "${key}" '%s' "${value}"
done < "${DEFAULTS_FILE}"

for key in \
    HAKO_DATA_MAX_ASSET_NUM \
    HAKO_PDU_CHANNEL_MAX \
    HAKO_RECV_EVENT_MAX \
    HAKO_SERVICE_CLIENT_MAX \
    HAKO_SERVICE_MAX \
    HAKO_CLIENT_NAMELEN_MAX \
    HAKO_SERVICE_NAMELEN_MAX
do
    if [ -z "${!key}" ]; then
        echo "ERROR: missing build defaults key: ${key}" >&2
        exit 1
    fi
done

echo "Build defaults: ${DEFAULTS_FILE}"

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
DEFAULT_HAKO_CLIENT_NAMELEN_MAX=${HAKO_CLIENT_NAMELEN_MAX}
if [ -n "${CLIENT_NAMELEN_MAX:-}" ] && [ "${CLIENT_NAMELEN_MAX}" -gt 0 ]; then
    :
else
    CLIENT_NAMELEN_MAX=${DEFAULT_HAKO_CLIENT_NAMELEN_MAX}
fi
DEFAULT_HAKO_SERVICE_NAMELEN_MAX=${HAKO_SERVICE_NAMELEN_MAX}
if [ -n "${SERVICE_NAMELEN_MAX:-}" ] && [ "${SERVICE_NAMELEN_MAX}" -gt 0 ]; then
    :
else
    SERVICE_NAMELEN_MAX=${DEFAULT_HAKO_SERVICE_NAMELEN_MAX}
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
echo "CLIENT_NAMELEN_MAX is ${CLIENT_NAMELEN_MAX}"
echo "SERVICE_NAMELEN_MAX is ${SERVICE_NAMELEN_MAX}"
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

BUILD_DIR="${HAKO_BUILD_DIR:-cmake-build}"
INSTALL_PREFIX="${HAKO_INSTALL_PREFIX:-/usr}"

if [ $# -eq 0 ]
then
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    if [ ${OS_TYPE} = "posix" ]
    then
        CMAKE_LOCAL_INSTALL_ARGS=()
        if [ -n "${HAKO_CORE_CONFIG_INSTALL_DIR:-}" ]; then
            CMAKE_LOCAL_INSTALL_ARGS+=("-DHAKO_CORE_CONFIG_INSTALL_DIR=${HAKO_CORE_CONFIG_INSTALL_DIR}")
        fi
        if [ -n "${HAKO_PYTHON_INSTALL_DIR:-}" ]; then
            CMAKE_LOCAL_INSTALL_ARGS+=("-DHAKO_PYTHON_INSTALL_DIR=${HAKO_PYTHON_INSTALL_DIR}")
        fi
        if [ -n "${HAKO_PYTHON_EXECUTABLE:-}" ]; then
            CMAKE_LOCAL_INSTALL_ARGS+=("-DHAKO_PYTHON_EXECUTABLE=${HAKO_PYTHON_EXECUTABLE}")
        fi
        if [ -n "${HAKO_PYTHON_WITH_SOABI:-}" ]; then
            CMAKE_LOCAL_INSTALL_ARGS+=("-DHAKO_PYTHON_WITH_SOABI=${HAKO_PYTHON_WITH_SOABI}")
        fi
        if [ -n "${HAKO_CORE_MMAP_PATH:-}" ]; then
            CMAKE_LOCAL_INSTALL_ARGS+=("-DHAKO_CORE_MMAP_PATH=${HAKO_CORE_MMAP_PATH}")
        fi
        if [ -n "${HAKO_ENABLE_GTEST:-}" ]; then
            CMAKE_LOCAL_INSTALL_ARGS+=("-DHAKO_ENABLE_GTEST=${HAKO_ENABLE_GTEST}")
        fi
        cmake "${SCRIPT_DIR}" -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" "${CMAKE_LOCAL_INSTALL_ARGS[@]}" $ENABLE_HAKO_TIME_MEASURE_FLAG \
            -DHAKO_BUILD_DEFAULTS_FILE="${DEFAULTS_FILE}" \
            -DHAKO_DATA_MAX_ASSET_NUM=${ASSET_NUM} \
            -DHAKO_SERVICE_MAX=${SERVICE_MAX} \
            -DHAKO_RECV_EVENT_MAX=${RECV_EVENT_MAX} \
            -DHAKO_SERVICE_CLIENT_MAX=${SERVICE_CLIENT_MAX} \
            -DHAKO_CLIENT_NAMELEN_MAX=${CLIENT_NAMELEN_MAX} \
            -DHAKO_SERVICE_NAMELEN_MAX=${SERVICE_NAMELEN_MAX} \
            -DHAKO_PDU_CHANNEL_MAX=${CHANNEL_MAX} \
            $BUILD_C_FLAGS
        make
    else
        cmake "${SCRIPT_DIR}"
        cmake --build . --target ALL_BUILD --config Release
    fi
else
    # ---- Clean mode ----
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR:?}"/*
fi
