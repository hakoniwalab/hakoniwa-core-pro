project(hako-examples-service-polling LANGUAGES CXX)

add_executable(polling_server polling_server.cpp)
add_executable(polling_client polling_client.cpp)
add_executable(polling_conductor conductor_main.cpp)
add_executable(callback_server callback_server.cpp)
add_executable(bench_server bench_server.cpp)
add_executable(polling_asset_server polling_asset_server.cpp)
add_executable(bench_callback_server bench_callback_server.cpp)
add_executable(bench_client bench_client.cpp)

foreach(target polling_server polling_client polling_asset_server bench_server bench_client)
    target_include_directories(${target} PRIVATE
        ${HAKO_INC_DIR}
        ${HAKO_PDU_SOURCE_DIR}
        ${HAKO_CORE_SOURCE_DIR}/src/include
    )
    target_link_libraries(${target} PRIVATE shakoc)
endforeach()

target_include_directories(polling_conductor PRIVATE ${HAKO_INC_DIR})
target_include_directories(callback_server PRIVATE ${HAKO_INC_DIR} ${HAKO_PDU_SOURCE_DIR} ${HAKO_CORE_SOURCE_DIR}/src/include)
target_link_libraries(callback_server PRIVATE assets)
target_include_directories(bench_callback_server PRIVATE ${HAKO_INC_DIR} ${HAKO_PDU_SOURCE_DIR} ${HAKO_CORE_SOURCE_DIR}/src/include)
target_link_libraries(bench_callback_server PRIVATE assets)
target_link_libraries(polling_conductor PRIVATE conductor)

if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(polling_server PRIVATE Threads::Threads)
    target_link_libraries(polling_client PRIVATE Threads::Threads)
    target_link_libraries(polling_conductor PRIVATE Threads::Threads)
endif()
