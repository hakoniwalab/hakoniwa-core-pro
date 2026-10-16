find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

include(GoogleTest)

add_executable(
    hako-core-pro-test
    hako_asset_pdu_compact_test.cpp
    hako_asset_state_check_test.cpp
    hako_command_runtime_probe_test.cpp
    hako_command_recovery_test.cpp
    hako_runtime_error_test.cpp
)

target_include_directories(
    hako-core-pro-test
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/src/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/src/hako
    PRIVATE ${PROJECT_SOURCE_DIR}/sources/assets/callback/src
    PRIVATE ${PROJECT_SOURCE_DIR}/sources/core/include
)

target_link_libraries(
    hako-core-pro-test
    assets
    hako-command-support
    nlohmann_json::nlohmann_json
    GTest::GTest
    GTest::Main
    Threads::Threads
)

gtest_add_tests(TARGET hako-core-pro-test TEST_LIST hako_core_pro_gtests)
set_tests_properties(${hako_core_pro_gtests} PROPERTIES TIMEOUT 15)

add_executable(hako-command-lock-holder hako_command_lock_holder.cpp)
add_executable(hako-runtime-c-api-probe hako_runtime_c_api_probe.cpp)
target_include_directories(
    hako-command-lock-holder
    PRIVATE ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/src/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/src/hako
    PRIVATE ${nlohmann_json_SOURCE_DIR}/single_include
)
target_link_libraries(hako-command-lock-holder hako Threads::Threads)
target_include_directories(
    hako-runtime-c-api-probe
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(hako-runtime-c-api-probe assets)
if(HAKO_CORE_SHARED_EFFECTIVE)
    # Windows + HAKO_CORE_SHARED: copy hako.dll next to the build-tree test
    # executables so ctest can load them.
    foreach(_hako_test_target IN ITEMS hako-core-pro-test hako-command-lock-holder hako-runtime-c-api-probe)
        add_custom_command(TARGET ${_hako_test_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:hako> $<TARGET_FILE_DIR:${_hako_test_target}>
        )
    endforeach()
endif()

set(_hako_lock_integration_python "${Python3_EXECUTABLE}")
set(_hako_lock_integration_binding_args)
if(TARGET hako_asset_python)
    set(_hako_lock_integration_python "${HAKO_PYTHON_RUNTIME_EXECUTABLE}")
    list(APPEND _hako_lock_integration_binding_args
        --hakopy-dir $<TARGET_FILE_DIR:hako_asset_python>
        --native-library-dir $<TARGET_FILE_DIR:assets>
        --hakopy-running-probe ${CMAKE_CURRENT_SOURCE_DIR}/../integration/hakopy_running_fail_stop_probe.py
    )
endif()
add_test(
    NAME HakoCommandLockIntegration
    COMMAND ${_hako_lock_integration_python}
        ${CMAKE_CURRENT_SOURCE_DIR}/../integration/hako_cmd_lock_integration.py
        --hako-cmd $<TARGET_FILE:hako-cmd>
        --holder $<TARGET_FILE:hako-command-lock-holder>
        --runtime-c-api-probe $<TARGET_FILE:hako-runtime-c-api-probe>
        ${_hako_lock_integration_binding_args}
)
set_tests_properties(HakoCommandLockIntegration PROPERTIES TIMEOUT 20)
