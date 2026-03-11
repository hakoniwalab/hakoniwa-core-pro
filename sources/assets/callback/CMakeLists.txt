set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hako-asset
    LANGUAGES C CXX
)

if (DEFINED HAKO_DATA_MAX_ASSET_NUM)
    add_compile_definitions(HAKO_DATA_MAX_ASSET_NUM=${HAKO_DATA_MAX_ASSET_NUM})
    message(STATUS "HAKO_DATA_MAX_ASSET_NUM is redefined with " ${HAKO_DATA_MAX_ASSET_NUM})
endif()
if (DEFINED HAKO_SERVICE_MAX)
    add_compile_definitions(HAKO_SERVICE_MAX=${HAKO_SERVICE_MAX})
    message(STATUS "HAKO_SERVICE_MAX is redefined with " ${HAKO_SERVICE_MAX})
endif()
if (DEFINED HAKO_RECV_EVENT_MAX)
    add_compile_definitions(HAKO_RECV_EVENT_MAX=${HAKO_RECV_EVENT_MAX})
    message(STATUS "HAKO_RECV_EVENT_MAX is redefined with " ${HAKO_RECV_EVENT_MAX})
endif()
if (DEFINED HAKO_SERVICE_CLIENT_MAX)
    add_compile_definitions(HAKO_SERVICE_CLIENT_MAX=${HAKO_SERVICE_CLIENT_MAX})
    message(STATUS "HAKO_SERVICE_CLIENT_MAX is redefined with " ${HAKO_SERVICE_CLIENT_MAX})
endif()
if (DEFINED HAKO_PDU_CHANNEL_MAX)
    add_compile_definitions(HAKO_PDU_CHANNEL_MAX=${HAKO_PDU_CHANNEL_MAX})
    message(STATUS "HAKO_PDU_CHANNEL_MAX is redefined with " ${HAKO_PDU_CHANNEL_MAX})
endif()

message(STATUS "CMAKE_VERSION:${CMAKE_VERSION}" )

set(OS_TYPE "posix")
if(WIN32)
    set(OS_TYPE "win")
endif(WIN32)
MESSAGE(STATUS "OS_TYPE=" ${OS_TYPE})

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz DOWNLOAD_EXTRACT_TIMESTAMP true)
FetchContent_MakeAvailable(json)




if(WIN32)
add_library(
    assets STATIC
    src/hako_asset.cpp
    src/hako_asset_impl.cpp
    ${HAKO_CORE_PRO_SOURCE_DIR}/assets/polling/src/hako_asset_service.cpp
    ${HAKO_CORE_PRO_SOURCE_DIR}/assets/polling/src/hako_asset_service_server.cpp
    ${HAKO_CORE_PRO_SOURCE_DIR}/assets/polling/src/hako_asset_service_client.cpp
)
else()
    # ENABLE_HAKO_TIME_MEASURE のオプションに基づいて、条件付きで src/hako_asset_impl_measure.cpp を追加
    if(ENABLE_HAKO_TIME_MEASURE)
        add_library(
            assets SHARED
            src/hako_asset.cpp
            src/hako_asset_impl.cpp
            src/hako_asset_impl_measure.cpp # 計測機能が有効な場合にビルド
        )
    else()
        add_library(
            assets SHARED
            src/hako_asset.cpp
            src/hako_asset_impl.cpp
            ${HAKO_CORE_PRO_SOURCE_DIR}/assets/polling/src/hako_asset_service.cpp
            ${HAKO_CORE_PRO_SOURCE_DIR}/assets/polling/src/hako_asset_service_server.cpp
            ${HAKO_CORE_PRO_SOURCE_DIR}/assets/polling/src/hako_asset_service_client.cpp
        )
    endif()
endif()

target_link_libraries(
    assets
    conductor
    hako
)
target_include_directories(
    assets
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INC_DIR}
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src/include
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src/hako
    PRIVATE ${HAKO_PDU_SOURCE_DIR}
    PRIVATE ${HAKO_CORE_PRO_SOURCE_DIR}/core/include
    PRIVATE ${HAKO_CORE_PRO_SOURCE_DIR}/core/src/service
    PRIVATE ${nlohmann_json_SOURCE_DIR}/single_include
)
set_target_properties(assets PROPERTIES VERSION 1.0.0 SOVERSION 1)

install(TARGETS assets
    EXPORT hakoniwa-core-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
