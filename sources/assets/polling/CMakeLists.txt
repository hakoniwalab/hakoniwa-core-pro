set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hako-client
    LANGUAGES C CXX
)

message(STATUS "CMAKE_VERSION:${CMAKE_VERSION}" )

set(OS_TYPE "posix")
if(WIN32)
    set(OS_TYPE "win")
endif(WIN32)
MESSAGE(STATUS "OS_TYPE=" ${OS_TYPE})

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz DOWNLOAD_EXTRACT_TIMESTAMP true)
FetchContent_MakeAvailable(json)

# ベースとなるソースファイルリスト
set(SHARED_SRC
    src/hako_capi.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/hako_asset_impl.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/hako_simevent_impl.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/hako_master_impl.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/core/simulation/time/hako_time.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/core/rpc/hako_internal_rpc.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/hako_sem_flock.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/hako_shared_memory_mmap.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/${OS_TYPE}/hako_mmap.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/${OS_TYPE}/hako_flock.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/hako_shared_memory_factory.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/${OS_TYPE}/os_file_io.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_string.cpp
    ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_logger.cpp
    ${HAKO_CORE_PRO_SOURCE_DIR}/core/src/hako_pro_data.cpp
    ${HAKO_CORE_PRO_SOURCE_DIR}/core/src/hako_pro_data_recv_event.cpp
    ${HAKO_CORE_PRO_SOURCE_DIR}/core/src/hako_pro_data_service.cpp
    ${HAKO_CORE_PRO_SOURCE_DIR}/core/src/hako_pro.cpp
)


add_library(
    shakoc SHARED
    ${SHARED_SRC}
)
# POSIX特有のファイルをWIN32ではない場合にのみ追加
if(NOT WIN32)
    target_sources(shakoc PRIVATE
        ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/posix/hako_shared_memory_shm.cpp
        ${HAKO_CORE_SOURCE_DIR}/src/hako/utils/hako_share/impl/posix/hako_sem.cpp
    )
endif()
if(MSVC)
elseif(WIN32)
    target_link_libraries(shakoc stdc++)
endif()

if (NOT WIN32)
    set(PTHREAD_LIB -pthread)
else()
    set(PTHREAD_LIB "")
endif()

target_link_libraries(
    shakoc 
    ${PTHREAD_LIB}
    ${HAKO_PYTHON_LIBS}
)


target_include_directories(
    shakoc
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src/include
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src/hako
    PRIVATE ${HAKO_PDU_SOURCE_DIR}
    PRIVATE ${nlohmann_json_SOURCE_DIR}/single_include
    PRIVATE ${HAKO_INC_DIR}
    PRIVATE ${HAKO_CORE_PRO_SOURCE_DIR}/core/include
)

