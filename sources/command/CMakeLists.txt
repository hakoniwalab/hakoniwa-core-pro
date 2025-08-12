set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-command
    LANGUAGES C CXX
)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz DOWNLOAD_EXTRACT_TIMESTAMP true)
FetchContent_MakeAvailable(json)

add_executable(
    hako-cmd
    src/dump_meta.cpp
    src/hako_cmd.cpp
)

target_link_libraries(
    hako-cmd 
    hako
)

if (NOT WIN32)
    target_link_libraries(
        hako-cmd 
        -pthread
    )
endif()

#message(STATUS "HAKO_INTERFACE_DIR=" ${HAKO_INTERFACE_DIR})

target_include_directories(
    hako-cmd 
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src/include
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src/hako
    PRIVATE ${nlohmann_json_SOURCE_DIR}/single_include
)
install(TARGETS hako-cmd DESTINATION bin)