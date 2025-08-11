set(HAKO_CMAKE_VERSION 3.20)
cmake_minimum_required(VERSION ${HAKO_CMAKE_VERSION})
cmake_policy(SET CMP0135 NEW)

project(hakoniwa-core-pro
    VERSION 1.0.0
    DESCRIPTION "hakoniwa-core-pro project"
    LANGUAGES C CXX
)

enable_testing()

set(HAKO_CORE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp")
set(HAKO_PDU_SOURCE_DIR "${PROJECT_SOURCE_DIR}/hakoniwa-ros2pdu/pdu/types")
set(HAKO_INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(HAKO_CORE_PRO_SOURCE_DIR "${PROJECT_SOURCE_DIR}/sources")

if (DEFINED HAKO_CLIENT_OPTION_FILEPATH)
    include(${HAKO_CLIENT_OPTION_FILEPATH})
else()
    include(cmake-options/default-cmake-options.cmake)
endif()

MESSAGE(STATUS "HAKO_CORE_EXTENSION: ${HAKO_CORE_EXTENSION}")

if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")

message(STATUS "BUILD_TYPE" ${BUILD_TYPE})
message(STATUS "GCOV" ${GCOV})

message(STATUS "CMAKE_C_FLAGS" ${CMAKE_C_FLAGS})
message(STATUS "CMAKE_CXX_FLAGS" ${CMAKE_CXX_FLAGS})

add_subdirectory(sources/core)
add_subdirectory(sources/command)
add_subdirectory(sources/assets/callback)
add_subdirectory(sources/assets/polling)
add_subdirectory(sources/conductor)
add_subdirectory(examples)
if(NOT "${CMAKE_CXX_FLAGS}" MATCHES "-m32")
add_subdirectory(sources/assets/bindings/python)
endif()


