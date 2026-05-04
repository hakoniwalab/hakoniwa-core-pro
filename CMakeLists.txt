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
set(HAKO_PDU_SOURCE_DIR "${PROJECT_SOURCE_DIR}/hakoniwa-pdu-registry/pdu/types")
set(HAKO_INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(HAKO_CORE_PRO_SOURCE_DIR "${PROJECT_SOURCE_DIR}/sources")
set(HAKO_BUILD_DEFAULTS_FILE "${PROJECT_SOURCE_DIR}/cmake/hako_build_defaults.conf")

function(hako_load_build_defaults defaults_file prefix)
    if(NOT EXISTS "${defaults_file}")
        message(FATAL_ERROR "Hakoniwa build defaults file not found: ${defaults_file}")
    endif()

    file(STRINGS "${defaults_file}" _hako_default_lines)
    foreach(_line IN LISTS _hako_default_lines)
        string(STRIP "${_line}" _line)
        if(_line STREQUAL "" OR _line MATCHES "^#")
            continue()
        endif()
        if(NOT _line MATCHES "^([A-Z0-9_]+)=([0-9]+)$")
            message(FATAL_ERROR "Invalid build default entry: ${_line}")
        endif()
        set(${prefix}${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" PARENT_SCOPE)
    endforeach()
endfunction()

function(hako_set_cache_default variable_name default_value docstring)
    if(NOT DEFINED ${variable_name})
        set(${variable_name} "${default_value}" CACHE STRING "${docstring}")
    endif()
endfunction()

hako_load_build_defaults("${HAKO_BUILD_DEFAULTS_FILE}" "HAKO_DEFAULT_")
hako_set_cache_default(HAKO_DATA_MAX_ASSET_NUM "${HAKO_DEFAULT_HAKO_DATA_MAX_ASSET_NUM}" "Maximum number of Hakoniwa assets")
hako_set_cache_default(HAKO_PDU_CHANNEL_MAX "${HAKO_DEFAULT_HAKO_PDU_CHANNEL_MAX}" "Maximum number of Hakoniwa PDU channels")
hako_set_cache_default(HAKO_RECV_EVENT_MAX "${HAKO_DEFAULT_HAKO_RECV_EVENT_MAX}" "Maximum number of Hakoniwa receive events")
hako_set_cache_default(HAKO_SERVICE_CLIENT_MAX "${HAKO_DEFAULT_HAKO_SERVICE_CLIENT_MAX}" "Maximum number of Hakoniwa service client channels")
hako_set_cache_default(HAKO_SERVICE_MAX "${HAKO_DEFAULT_HAKO_SERVICE_MAX}" "Maximum number of Hakoniwa services")
hako_set_cache_default(HAKO_CLIENT_NAMELEN_MAX "${HAKO_DEFAULT_HAKO_CLIENT_NAMELEN_MAX}" "Maximum client name length in Hakoniwa service tables")
hako_set_cache_default(HAKO_SERVICE_NAMELEN_MAX "${HAKO_DEFAULT_HAKO_SERVICE_NAMELEN_MAX}" "Maximum service name length in Hakoniwa service tables")

message(STATUS
    "Hakoniwa build limits: "
    "asset_num=${HAKO_DATA_MAX_ASSET_NUM} "
    "channel_max=${HAKO_PDU_CHANNEL_MAX} "
    "recv_event_max=${HAKO_RECV_EVENT_MAX} "
    "service_client_max=${HAKO_SERVICE_CLIENT_MAX} "
    "service_max=${HAKO_SERVICE_MAX} "
    "client_namelen_max=${HAKO_CLIENT_NAMELEN_MAX} "
    "service_namelen_max=${HAKO_SERVICE_NAMELEN_MAX}"
)

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
option(HAKO_CORE_PRO_BUILD_COMMAND "Build hakoniwa-core-pro command tools" ON)
if(HAKO_CORE_PRO_BUILD_COMMAND)
add_subdirectory(sources/command)
endif()
add_subdirectory(sources/assets/callback)
add_subdirectory(sources/assets/polling)
add_subdirectory(sources/conductor)
option(HAKO_CORE_PRO_BUILD_EXAMPLES "Build hakoniwa-core-pro examples" ON)
if(HAKO_CORE_PRO_BUILD_EXAMPLES)
add_subdirectory(examples)
endif()
option(HAKO_CORE_PRO_BUILD_PYTHON_BINDINGS "Build hakoniwa-core-pro python bindings" ON)
if(HAKO_CORE_PRO_BUILD_PYTHON_BINDINGS AND NOT "${CMAKE_CXX_FLAGS}" MATCHES "-m32")
add_subdirectory(sources/assets/bindings/python)
endif()
option(HAKO_ENABLE_GTEST "Enable GTest-based tests for hakoniwa-core-pro" ON)
if(HAKO_ENABLE_GTEST)
add_subdirectory(tests/cpp)
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
    DIRECTORY ${PROJECT_SOURCE_DIR}/hakoniwa-pdu-registry/pdu/types/
    DESTINATION include/hakoniwa/pdu
)

# Install core config file
install(
    FILES ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/cpp_core_config.json
    DESTINATION /etc/hakoniwa
)

# Install PDU offset files
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/hakoniwa-pdu-registry/pdu/offset/
    DESTINATION share/hakoniwa/offset
)
