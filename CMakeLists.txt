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
    MESSAGE(STATUS "Using custom CMake options from ${HAKO_CLIENT_OPTION_FILEPATH}")
    include(${HAKO_CLIENT_OPTION_FILEPATH})
else()
    MESSAGE(STATUS "Using default CMake options")
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
#add_subdirectory(examples)
if(NOT "${CMAKE_CXX_FLAGS}" MATCHES "-m32")
add_subdirectory(sources/assets/bindings/python)
endif()

# For pkg-config support
include(GNUInstallDirs)
configure_file(
  cmake/hakoniwa-core.pc.in
  ${CMAKE_BINARY_DIR}/hakoniwa-core.pc
  @ONLY
)
install(
  FILES ${CMAKE_BINARY_DIR}/hakoniwa-core.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# For find_package support
include(CMakePackageConfigHelpers)

install(EXPORT hakoniwa-core-targets
  FILE hakoniwa-core-targets.cmake
  NAMESPACE hakoniwa-core::
  DESTINATION lib/cmake/hakoniwa-core
)

configure_package_config_file(
  cmake/hakoniwa-coreConfig.cmake.in
  ${CMAKE_BINARY_DIR}/hakoniwa-coreConfig.cmake
  INSTALL_DESTINATION lib/cmake/hakoniwa-core
  PATH_VARS CMAKE_INSTALL_PREFIX
)

install(
  FILES ${CMAKE_BINARY_DIR}/hakoniwa-coreConfig.cmake
  DESTINATION lib/cmake/hakoniwa-core
)

# Install hakonwia core header files
file(GLOB HAKO_ROOT_HEADERS
    "${PROJECT_SOURCE_DIR}/include/*.h*"
)
install(
  FILES ${HAKO_ROOT_HEADERS}
  DESTINATION include/hakoniwa
)

# Install PDU header files
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/hakoniwa-ros2pdu/pdu/types/
    DESTINATION include/hakoniwa/pdu
)

# Install core config file
install(
    FILES ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/cpp_core_config.json
    DESTINATION /etc/hakoniwa
)

# Install PDU offset files
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/hakoniwa-ros2pdu/pdu/offset/
    DESTINATION share/hakoniwa/offset
)