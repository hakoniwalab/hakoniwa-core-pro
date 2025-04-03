set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hako-conductor
    LANGUAGES C CXX
)

message(STATUS "CMAKE_VERSION:${CMAKE_VERSION}" )

set(OS_TYPE "posix")
if(WIN32)
    set(OS_TYPE "win")
endif(WIN32)
MESSAGE(STATUS "OS_TYPE=" ${OS_TYPE})


if(WIN32)
add_library(
    conductor STATIC
    src/hako_conductor.cpp
    src/hako_conductor_impl.cpp
)
else()
add_library(
    conductor SHARED
    src/hako_conductor.cpp
    src/hako_conductor_impl.cpp
)
endif()
target_link_libraries(
    conductor 
    hako
)

target_include_directories(
    conductor
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INC_DIR}
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src/include
    PRIVATE ${HAKO_CORE_SOURCE_DIR}/src/hako
    PRIVATE ${nlohmann_json_SOURCE_DIR}/single_include
)
