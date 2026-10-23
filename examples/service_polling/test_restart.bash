#!/bin/bash
# Reproduction test for hakoniwa-core-pro#109: a service server must keep answering
# after the simulation is started again in the same processes.
# Usage (from the repository root, after building): bash examples/service_polling/test_restart.bash
set -u

BUILD_DIR=${BUILD_DIR:-./cmake-build}
BIN=${BUILD_DIR}/examples/service_polling
HAKO_CMD=${BUILD_DIR}/sources/command/hako-cmd
EXAMPLE_DIR=./examples/service_polling
SERVICE_JSON=${EXAMPLE_DIR}/service.json
PDU_JSON=${EXAMPLE_DIR}/pdu_config.json
WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/hako-restart-test.XXXXXX")

mkdir -p "${WORK_DIR}/mmap"
cat > "${WORK_DIR}/cpp_core_config.json" <<JSON
{
  "shm_type": "mmap",
  "core_mmap_path": "${WORK_DIR}/mmap",
  "asset_timeout_usec": 600000000
}
JSON
export HAKO_CONFIG_PATH="${WORK_DIR}/cpp_core_config.json"
export DYLD_LIBRARY_PATH=${BUILD_DIR}/sources/assets/polling:${BUILD_DIR}/sources/assets/callback:${BUILD_DIR}/sources/conductor
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

# wait_results LOG COUNT TIMEOUT_SEC: wait until LOG has COUNT "RESULT" lines
wait_results() {
    local log=$1 count=$2 timeout=$3 elapsed=0
    while [ "${elapsed}" -lt "$((timeout * 10))" ]; do
        [ "$(grep -c '^RESULT' "${log}" 2>/dev/null)" -ge "${count}" ] && return 0
        sleep 0.1
        elapsed=$((elapsed + 1))
    done
    return 1
}

# run_doing_case NAME SERVER_KIND [SERVER_OPTION]: the server holds the first request,
# so it is DOING when the simulation is stopped and reset; after the next start a new
# request must be answered (correctly: polling_client checks the sum).
# --late-reply also makes the server write the held request's marked response (sum=999)
# after the restart; it must not reach the new request.
run_doing_case() {
    local name=$1 server_kind=$2 server_option=${3:---hold-first-request}
    local log_dir="${WORK_DIR}/${name}"
    mkdir -p "${log_dir}"
    "${BIN}/polling_conductor" > "${log_dir}/conductor.log" 2>&1 & PIDS+=($!)
    sleep 1
    if [ "${server_kind}" = "callback" ]; then
        "${BIN}/callback_server" "${PDU_JSON}" "${SERVICE_JSON}" "${server_option}" > "${log_dir}/server.log" 2>&1 & PIDS+=($!)
    else
        "${BIN}/polling_asset_server" "${SERVICE_JSON}" "${server_option}" > "${log_dir}/server.log" 2>&1 & PIDS+=($!)
    fi
    sleep 1
    "${BIN}/polling_client" "${SERVICE_JSON}" 3 > "${log_dir}/client.log" 2>&1 & PIDS+=($!)
    sleep 1

    local result="PASS" detail=""
    "${HAKO_CMD}" start > /dev/null 2>&1
    sleep 1
    if ! grep -q "holding the first request" "${log_dir}/server.log"; then
        result="FAIL"; detail="the server did not get into DOING"
    else
        "${HAKO_CMD}" stop > /dev/null 2>&1
        sleep 2
        "${HAKO_CMD}" reset > /dev/null 2>&1
        sleep 2
        "${HAKO_CMD}" start > /dev/null 2>&1
        if ! wait_results "${log_dir}/client.log" 1 10; then
            result="FAIL"; detail="no response after the restart"
        fi
    fi
    cleanup
    echo "${result} ${name}${detail:+ (${detail})}  logs: ${log_dir}"
    [ "${result}" = "PASS" ]
}

# run_case NAME SERVER_KIND SEQUENCE: SEQUENCE is "stop" or "stop-reset"
run_case() {
    local name=$1 server_kind=$2 sequence=$3
    local log_dir="${WORK_DIR}/${name}"
    mkdir -p "${log_dir}"
    "${BIN}/polling_conductor" > "${log_dir}/conductor.log" 2>&1 & PIDS+=($!)
    sleep 1
    if [ "${server_kind}" = "asset" ]; then
        "${BIN}/polling_asset_server" "${SERVICE_JSON}" > "${log_dir}/server.log" 2>&1 & PIDS+=($!)
    elif [ "${server_kind}" = "callback" ]; then
        "${BIN}/callback_server" "${PDU_JSON}" "${SERVICE_JSON}" > "${log_dir}/server.log" 2>&1 & PIDS+=($!)
    else
        "${BIN}/polling_server" "${SERVICE_JSON}" > "${log_dir}/server.log" 2>&1 & PIDS+=($!)
    fi
    sleep 1
    "${BIN}/polling_client" "${SERVICE_JSON}" 3 > "${log_dir}/client.log" 2>&1 & PIDS+=($!)
    sleep 1

    local result="PASS" detail=""
    "${HAKO_CMD}" start > /dev/null 2>&1
    if ! wait_results "${log_dir}/client.log" 1 10; then
        result="FAIL"; detail="no response in the first run"
    else
        "${HAKO_CMD}" stop > /dev/null 2>&1
        sleep 2
        if [ "${sequence}" = "stop-reset" ]; then
            "${HAKO_CMD}" reset > /dev/null 2>&1
            sleep 2
        fi
        "${HAKO_CMD}" start > /dev/null 2>&1
        if ! wait_results "${log_dir}/client.log" 2 10; then
            result="FAIL"; detail="no response after the second start"
        fi
    fi
    cleanup
    echo "${result} ${name}${detail:+ (${detail})}  logs: ${log_dir}"
    [ "${result}" = "PASS" ]
}

failed=0
run_case callback-stop-reset-start callback stop-reset || failed=1
run_case asset-stop-start asset stop || failed=1
run_case asset-stop-reset-start asset stop-reset || failed=1
run_doing_case asset-doing-restart asset || failed=1
run_doing_case callback-doing-restart callback || failed=1
run_doing_case asset-late-reply asset --late-reply || failed=1
run_doing_case callback-late-reply callback --late-reply || failed=1
exit ${failed}
