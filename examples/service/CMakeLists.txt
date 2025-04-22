set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hako-examples-service
    LANGUAGES C CXX
)

message(STATUS "CMAKE_VERSION:${CMAKE_VERSION}" )

set(OS_TYPE "posix")
if(WIN32)
    set(OS_TYPE "win")
endif(WIN32)
MESSAGE(STATUS "OS_TYPE=" ${OS_TYPE})

add_executable(
    server
    src/asset_server.cpp
)

add_executable(
    client
    src/asset_client.cpp
)

target_include_directories(
    server
    PRIVATE src/include
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INC_DIR}
    PRIVATE ${HAKO_PDU_SOURCE_DIR}
)
target_include_directories(
    client
    PRIVATE src/include
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INC_DIR}
    PRIVATE ${HAKO_PDU_SOURCE_DIR}
    PRIVATE ${HAKO_CORE_PRO_SOURCE_DIR}/core/include
)

target_link_libraries(
    server
    assets
    conductor
)
target_link_libraries(
    client
    assets
)
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(
        server
        Threads::Threads
    )
    target_link_libraries(
        client
        Threads::Threads
    )
endif()