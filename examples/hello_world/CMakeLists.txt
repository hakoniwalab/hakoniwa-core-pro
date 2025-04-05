set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hako-examples-hello_world
    LANGUAGES C CXX
)

message(STATUS "CMAKE_VERSION:${CMAKE_VERSION}" )

set(OS_TYPE "posix")
if(WIN32)
    set(OS_TYPE "win")
endif(WIN32)
MESSAGE(STATUS "OS_TYPE=" ${OS_TYPE})

add_executable(
    hello_world
    src/hello_world.c
)

target_include_directories(
    hello_world
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INC_DIR}
    PRIVATE ${HAKO_INC_DIR}
)

target_link_libraries(
    hello_world
    assets
    conductor
)
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(
        hello_world
        Threads::Threads
    )
endif()
