set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hako-examples-pdu-communication
    LANGUAGES C CXX
)

message(STATUS "CMAKE_VERSION:${CMAKE_VERSION}" )

set(OS_TYPE "posix")
if(WIN32)
    set(OS_TYPE "win")
endif(WIN32)
MESSAGE(STATUS "OS_TYPE=" ${OS_TYPE})

add_executable(
    asset_plant
    src/asset_plant.c
)

add_executable(
    asset_controller
    src/asset_controller.c
)

target_include_directories(
    asset_plant
    PRIVATE src/include
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INC_DIR}
)
target_include_directories(
    asset_controller
    PRIVATE src/include
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INC_DIR}
)

target_link_libraries(
    asset_plant
    assets
    conductor
)
target_link_libraries(
    asset_controller
    assets
)
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(
        asset_plant
        Threads::Threads
    )
    target_link_libraries(
        asset_controller
        Threads::Threads
    )
endif()