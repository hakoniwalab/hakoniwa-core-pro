find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include(GoogleTest)

add_executable(
    hako-core-pro-test
    hako_asset_pdu_compact_test.cpp
)

target_include_directories(
    hako-core-pro-test
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hakoniwa-core-cpp/src/include
)

target_link_libraries(
    hako-core-pro-test
    assets
    GTest::GTest
    GTest::Main
    Threads::Threads
)

gtest_add_tests(TARGET hako-core-pro-test)
