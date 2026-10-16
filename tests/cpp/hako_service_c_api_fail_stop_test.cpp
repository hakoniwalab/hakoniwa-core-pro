#include "hako.hpp"
#include "hako_asset_service.h"
#include "hako_runtime_error.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

bool set_config_path(const std::filesystem::path& path)
{
#ifdef _WIN32
    return _putenv_s("HAKO_CONFIG_PATH", path.string().c_str()) == 0;
#else
    return setenv("HAKO_CONFIG_PATH", path.string().c_str(), 1) == 0;
#endif
}

bool flip_last_byte(const std::filesystem::path& path)
{
    std::fstream stream(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!stream) {
        return false;
    }
    stream.seekg(-1, std::ios::end);
    char value = 0;
    stream.read(&value, 1);
    if (!stream) {
        return false;
    }
    value ^= 1;
    stream.seekp(-1, std::ios::end);
    stream.write(&value, 1);
    stream.flush();
    return static_cast<bool>(stream);
}

} // namespace

int main()
{
    const auto unique = std::chrono::steady_clock::now()
                            .time_since_epoch()
                            .count();
    const auto root = std::filesystem::temp_directory_path() /
        ("hako-service-c-api-fail-stop-" + std::to_string(unique));
    std::filesystem::create_directories(root);

    const auto core_config = root / "cpp_core_config.json";
    {
        std::ofstream output(core_config);
        output << "{\"shm_type\":\"mmap\",\"core_mmap_path\":\""
               << root.generic_string() << "\"}\n";
    }
    const auto service_config = root / "service.json";
    {
        std::ofstream output(service_config);
        output
            << "{\"pduMetaDataSize\":8,\"services\":[{"
            << "\"name\":\"Probe\",\"type\":\"test/Probe\","
            << "\"maxClients\":1,\"pduSize\":{"
            << "\"server\":{\"baseSize\":8,\"heapSize\":0},"
            << "\"client\":{\"baseSize\":8,\"heapSize\":0}}}]}\n";
    }
    if (!set_config_path(core_config) || !hako::init() ||
        hako::create_master() == nullptr) {
        std::cerr << "failed to initialize native master fixture" << std::endl;
        return 2;
    }

    const auto flock = root / "flock.bin";
    if (!flip_last_byte(flock)) {
        std::cerr << "failed to corrupt master-lock metadata" << std::endl;
        return 3;
    }

    const int result =
        hako_asset_service_initialize(service_config.string().c_str());
    const auto status = hako_runtime_last_error_status();
    const std::string message = hako_runtime_last_error();

    // Restore the checksum byte before normal Core teardown.
    if (!flip_last_byte(flock)) {
        std::cerr << "failed to restore master-lock metadata" << std::endl;
        return 4;
    }
    hako::destroy();
    std::error_code cleanup_error;
    std::filesystem::remove_all(root, cleanup_error);

    if (result != -1 || status != HAKO_RUNTIME_ERROR_UNKNOWN ||
        message.empty()) {
        std::cerr << "service C API did not contain the native exception:"
                  << " result=" << result
                  << " status=" << hako_runtime_error_status_name(status)
                  << " message=" << message << std::endl;
        return 5;
    }
    std::cout << "SERVICE_C_API_FAIL_STOP_OK" << std::endl;
    return 0;
}
