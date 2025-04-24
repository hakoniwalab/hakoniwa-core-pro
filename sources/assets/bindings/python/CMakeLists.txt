cmake_minimum_required(VERSION 3.12)

project(hakoniwa_python_project
    LANGUAGES C CXX
)

# モダンCMakeのみサポート
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    message(FATAL_ERROR "CMake 3.12 or higher is required.")
endif()

add_library(
    hako_asset_python SHARED
    src/hako_asset_python.cpp
)
# OSごとに分岐
if(WIN32)
    message(STATUS "Configuring for Windows")

    # Windows固有の設定
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    set(HAKO_PYTHON_LIBS ${Python_LIBRARIES})

    target_compile_options(
        hako_asset_python PRIVATE
        /wd4514 /wd4100 # Windows特有の警告無効化
    )

elseif(UNIX)
    message(STATUS "Configuring for Unix-like system (Ubuntu/Mac)")

    # Unix固有の設定
    execute_process(
        COMMAND pyenv which python
        RESULT_VARIABLE PYENV_FOUND
        OUTPUT_VARIABLE PYENV_PYTHON_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message(STATUS "PYENV_FOUND:${PYENV_FOUND}")

    if(PYENV_FOUND EQUAL 0)
        message(STATUS "pyenv detected, using pyenv Python.")
        set(Python_EXECUTABLE ${PYENV_PYTHON_PATH})
        set(Python_ROOT_DIR ${PYENV_PYTHON_PATH})
    else()
        message(WARNING "pyenv was not found. Falling back to system Python.")
    endif()

    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    include_directories(${Python_INCLUDE_DIRS})
    set(HAKO_PYTHON_LIBS ${Python_LIBRARIES})

    target_compile_options(
        hako_asset_python PRIVATE
        -Wall -Wextra # Unix系の警告設定
    )
else()
    message(FATAL_ERROR "Unknown platform!")
endif()

message(STATUS "CMAKE_VERSION:${CMAKE_VERSION}" )
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
    hako_asset_python 
    assets
    ${HAKO_PYTHON_LIBS}
)
