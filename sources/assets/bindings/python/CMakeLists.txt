cmake_minimum_required(VERSION 3.12)

project(hakoniwa_python_project
    LANGUAGES C CXX
)

# モダンCMakeのみサポート
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    message(FATAL_ERROR "CMake 3.12 or higher is required.")
endif()

set(
    HAKO_PYTHON_EXECUTABLE
    ""
    CACHE FILEPATH
    "Explicit Python interpreter used to build hakopy"
)
option(
    HAKO_PYTHON_WITH_SOABI
    "Build hakopy as a standard SOABI-tagged Python extension module"
    OFF
)

if(HAKO_PYTHON_EXECUTABLE)
    message(STATUS "Using explicitly selected Python: ${HAKO_PYTHON_EXECUTABLE}")
    set(Python_EXECUTABLE "${HAKO_PYTHON_EXECUTABLE}" CACHE FILEPATH "" FORCE)
elseif(UNIX)
    execute_process(
        COMMAND pyenv which python
        RESULT_VARIABLE PYENV_FOUND
        OUTPUT_VARIABLE PYENV_PYTHON_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message(STATUS "PYENV_FOUND:${PYENV_FOUND}")

    if(PYENV_FOUND EQUAL 0)
        message(STATUS "pyenv detected, using pyenv Python.")
        set(Python_EXECUTABLE "${PYENV_PYTHON_PATH}" CACHE FILEPATH "" FORCE)
    else()
        message(WARNING "pyenv was not found. Falling back to system Python.")
    endif()
endif()

if(HAKO_PYTHON_WITH_SOABI)
    find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
    Python_add_library(
        hako_asset_python MODULE WITH_SOABI
        src/hako_asset_python.cpp
    )
    set(HAKO_PYTHON_LIBS Python::Module)
else()
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    add_library(
        hako_asset_python SHARED
        src/hako_asset_python.cpp
    )
    set(HAKO_PYTHON_LIBS ${Python_LIBRARIES})
endif()

# OSごとに分岐
if(WIN32)
    message(STATUS "Configuring for Windows")
    target_compile_options(
        hako_asset_python PRIVATE
        /wd4514 /wd4100 # Windows特有の警告無効化
    )
elseif(UNIX)
    message(STATUS "Configuring for Unix-like system (Ubuntu/Mac)")
    include_directories(${Python_INCLUDE_DIRS})
    target_compile_options(
        hako_asset_python PRIVATE
        -Wall -Wextra # Unix系の警告設定
    )
else()
    message(FATAL_ERROR "Unknown platform!")
endif()

message(STATUS "CMAKE_VERSION:${CMAKE_VERSION}" )
message(STATUS "HAKO_PYTHON_WITH_SOABI:${HAKO_PYTHON_WITH_SOABI}" )
message(STATUS "Python_EXECUTABLE:${Python_EXECUTABLE}" )
message(STATUS "Python_VERSION:${Python_VERSION}" )
message(STATUS "Python_SOABI:${Python_SOABI}" )
message(STATUS "PYTHON_INCLUDE_DIRS:${PYTHON_INCLUDE_DIRS}" )
message(STATUS "Python_INCLUDE_DIRS:${Python_INCLUDE_DIRS}" )
message(STATUS "PYTHON_LIBRARIES:${PYTHON_LIBRARIES}" )
message(STATUS "Python_LIBRARIES:${Python_LIBRARIES}" )
message(STATUS "Python_LIBRARY_DIRS:${Python_LIBRARY_DIRS}" )



# インクルードディレクトリの設定
target_include_directories(
    hako_asset_python PRIVATE
    ${Python_INCLUDE_DIRS}
    ${HAKO_INC_DIR}
    ${nlohmann_json_SOURCE_DIR}/single_include
    ${HAKO_CORE_SOURCE_DIR}/src/hako
    ${HAKO_CORE_SOURCE_DIR}/src/include
    ${HAKO_CORE_PRO_SOURCE_DIR}/core/include
)

# リンクライブラリの設定
target_link_libraries(
    hako_asset_python PRIVATE
    assets
    ${HAKO_PYTHON_LIBS}
)

# 出力されるファイル名をlibhako_asset_python.soからhakopy.soに変更
set_target_properties(hako_asset_python PROPERTIES
    PREFIX ""  # lib 前置きを消す
    OUTPUT_NAME "hakopy"
)

# Legacy mode keeps the historical untagged extension filename. SOABI mode
# leaves the suffix selected by Python_add_library(... WITH_SOABI).
# - Windows: .pyd
# - macOS: .so
# - Linux: .so (デフォルト)
if(NOT HAKO_PYTHON_WITH_SOABI)
    if(WIN32)
        set_target_properties(hako_asset_python PROPERTIES SUFFIX ".pyd")
    elseif(APPLE)
        set_target_properties(hako_asset_python PROPERTIES SUFFIX ".so")
    endif()
endif()

# macOS用のrpath設定: hakopy.soが依存するdylibを見つけられるようにする
if(APPLE)
    set_target_properties(hako_asset_python PROPERTIES
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif()

# ライブラリをインストールする設定
set(
    HAKO_PYTHON_INSTALL_DIR
    "${Python_SITELIB}"
    CACHE PATH
    "Install destination for hakopy (absolute or relative to CMAKE_INSTALL_PREFIX)"
)
install(TARGETS hako_asset_python
    LIBRARY DESTINATION "${HAKO_PYTHON_INSTALL_DIR}"
    RUNTIME DESTINATION "${HAKO_PYTHON_INSTALL_DIR}"
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/hakoniwa_measurement"
    DESTINATION "${HAKO_PYTHON_INSTALL_DIR}"
    FILES_MATCHING PATTERN "*.py"
)
