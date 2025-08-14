@echo off

setlocal

rem Set default asset number if not provided by environment variable
set DEFAULT_HAKO_ASSET_NUM=4
if not defined ASSET_NUM (
    set ASSET_NUM=%DEFAULT_HAKO_ASSET_NUM%
)
echo ASSET_NUM is %ASSET_NUM%

rem Clean build directory if any argument is provided
if not "%1"=="" (
    echo Cleaning cmake-build directory...
    if exist cmake-build (
        rmdir /s /q cmake-build
    )
    goto :eof
)

rem Create build directory if it does not exist
if not exist cmake-build (
    mkdir cmake-build
)

rem Change to build directory and run cmake and build
cd cmake-build
cmake .. -G "Visual Studio 17 2022" -A x64 -DHAKO_CLIENT_OPTION_FILEPATH="cmake-options\win-cmake-options.cmake" -DHAKO_DATA_MAX_ASSET_NUM=%ASSET_NUM%
cmake --build . --config Release

endlocal
