set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hako-asset
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




if(WIN32)
add_library(
    assets STATIC
    src/hako_asset.cpp
    src/hako_asset_impl.cpp
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
            src/hako_service.cpp
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
    PRIVATE ${HAKO_CORE_PRO_SOURCE_DIR}/core/include
    PRIVATE ${nlohmann_json_SOURCE_DIR}/single_include
)
