find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

include(GoogleTest)

add_executable(
    hako-core-pro-test
    hako_asset_pdu_compact_test.cpp
    hako_asset_state_check_test.cpp
    hako_command_runtime_probe_test.cpp
)

target_include_directories(
    hako-core-pro-test
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/src/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/src/hako
    PRIVATE ${PROJECT_SOURCE_DIR}/sources/assets/callback/src
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

gtest_add_tests(TARGET hako-core-pro-test)

add_executable(hako-command-lock-holder hako_command_lock_holder.cpp)
target_include_directories(
    hako-command-lock-holder
    PRIVATE ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/src/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/src/hako
)
target_link_libraries(hako-command-lock-holder hako Threads::Threads)

add_test(
    NAME HakoCommandLockIntegration
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../integration/hako_cmd_lock_integration.py
        --hako-cmd $<TARGET_FILE:hako-cmd>
        --holder $<TARGET_FILE:hako-command-lock-holder>
)
set_tests_properties(HakoCommandLockIntegration PROPERTIES TIMEOUT 20)
