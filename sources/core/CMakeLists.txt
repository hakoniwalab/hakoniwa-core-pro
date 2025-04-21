set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-core-pro
    LANGUAGES C CXX
)
if (DEFINED HAKO_DATA_MAX_ASSET_NUM)
    add_compile_definitions(HAKO_DATA_MAX_ASSET_NUM=${HAKO_DATA_MAX_ASSET_NUM})
    message(STATUS "HAKO_DATA_MAX_ASSET_NUM is redefined with " ${HAKO_DATA_MAX_ASSET_NUM})
endif()

set(OS_TYPE "posix")
if(WIN32)
    set(OS_TYPE "win")
elseif(MSYS)
    set(OS_TYPE "win")
endif(WIN32)
MESSAGE(STATUS "OS_TYPE=" ${OS_TYPE})

include(FetchContent)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "/etc/os-release")
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
else()
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
endif()
FetchContent_MakeAvailable(json)

add_library(
    hako STATIC
    ${HAKO_CORE_SOURCE_DIR}/src/hako/hako_asset_impl.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/hako_simevent_impl.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/hako_master_impl.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/core/simulation/time/hako_time.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/core/rpc/hako_internal_rpc.cpp
    #for filelock && mmap
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/hako_sem_flock.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/hako_shared_memory_mmap.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/${OS_TYPE}/hako_mmap.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/${OS_TYPE}/hako_flock.cpp

    #for sem && shared memory
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/hako_shared_memory_factory.cpp
    
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/${OS_TYPE}/os_file_io.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_string.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_logger.cpp
    src/hako_pro_data.cpp
    src/hako_pro_data_recv_event.cpp
    src/hako_pro_data_service.cpp
    src/hako_service_impl.cpp
    src/hako_service_protocol.cpp
    src/hako_pro.cpp
)
# POSIX特有のファイルをWIN32ではない場合にのみ追加
if(NOT WIN32)
    target_sources(hako PRIVATE
        ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/posix/hako_shared_memory_shm.cpp
        ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/posix/hako_sem.cpp
    )
endif()

target_include_directories(
    hako
    PRIVATE /usr/local/include
    PRIVATE /mingw64/include
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src/include
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src//hako
    PRIVATE ${HAKO_PDU_SOURCE_DIR}
    PRIVATE ${nlohmann_json_SOURCE_DIR}/single_include
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)
if (MSVC)
elseif(WIN32)
    target_link_libraries(hako stdc++)
else()
endif()
