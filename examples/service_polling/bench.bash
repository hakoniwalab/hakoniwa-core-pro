#!/bin/bash
# Stage 5: Hakoniwa-only service (RPC) cost for image-sized responses.
# Busy loops on both sides (no sleep). Usage from the repository root after building:
#   bash examples/service_polling/bench.bash [count]
set -u

COUNT=${1:-300}
BUILD_DIR=${BUILD_DIR:-./cmake-build}
BIN=${BUILD_DIR}/examples/service_polling
HAKO_CMD=${BUILD_DIR}/sources/command/hako-cmd
WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/hako-bench.XXXXXX")
SIZES=${SIZES:-"0x0 128x170 640x480 1280x720 1920x1080"}

mkdir -p "${WORK_DIR}/mmap"
cat > "${WORK_DIR}/cpp_core_config.json" <<JSON
{
  "shm_type": "mmap",
  "core_mmap_path": "${WORK_DIR}/mmap",
  "asset_timeout_usec": 600000000
}
JSON
export HAKO_CONFIG_PATH="${WORK_DIR}/cpp_core_config.json"
export DYLD_LIBRARY_PATH=${BUILD_DIR}/sources/assets/polling:${BUILD_DIR}/sources/conductor
export LD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}

PIDS=()
cleanup() {
    for pid in "${PIDS[@]:-}"; do
        [ -n "${pid}" ] && kill "${pid}" 2>/dev/null
    done
    wait 2>/dev/null
    PIDS=()
}
trap cleanup EXIT

for size in ${SIZES}; do
    width=${size%x*}
    height=${size#*x}
    bytes=$((width * height * 3))
    dir="${WORK_DIR}/${size}"
    mkdir -p "${dir}"
    # Base sizes of hako_srv_msgs/RenderCamera{Request,Response}Packet from the registry.
    cat > "${dir}/service.json" <<JSON
{
    "pduMetaDataSize": 24,
    "services": [
        {
            "name": "HakoCamera/RenderCamera",
            "type": "hako_srv_msgs/RenderCamera",
            "maxClients": 1,
            "pduSize": {
                "server": { "heapSize": 256, "baseSize": 464 },
                "client": { "heapSize": $((bytes + 1024)), "baseSize": 576 }
            }
        }
    ],
    "nodes": []
}
JSON
    "${BIN}/polling_conductor" > "${dir}/conductor.log" 2>&1 & PIDS+=($!)
    sleep 1
    "${BIN}/bench_server" "${dir}/service.json" "${dir}/server.csv" > "${dir}/server.log" 2>&1 & PIDS+=($!)
    sleep 1
    "${BIN}/bench_client" "${dir}/service.json" "${size}" "${COUNT}" "${dir}/client.csv" > "${dir}/client.log" 2>&1 & CLIENT=$!
    PIDS+=(${CLIENT})
    sleep 1
    "${HAKO_CMD}" start > /dev/null 2>&1
    wait "${CLIENT}"
    "${HAKO_CMD}" stop > /dev/null 2>&1
    cleanup
    grep -E "^MEMCPY|^ERROR" "${dir}/client.log"
done
python3 "$(dirname "$0")/bench_summary.py" "${WORK_DIR}" ${SIZES}
echo "logs: ${WORK_DIR}"
