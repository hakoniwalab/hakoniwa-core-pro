#include "hako_asset.h"
#include "runtime_recovery.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace {

bool wait_for(const std::filesystem::path& path, int seconds = 10)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        if (std::filesystem::exists(path)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

void signal_ready(const std::filesystem::path& path)
{
    std::ofstream(path) << "ready\n";
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 4) {
        return 2;
    }
    const std::string mode = argv[1];
    const std::filesystem::path ready_path = argv[2];
    const std::filesystem::path release_path = argv[3];

    if (mode == "external") {
        const int result = hako_initialize_for_external();
        if (result != 0) {
            std::cerr << "external initialization failed: "
                      << hako_asset_last_error() << std::endl;
            return 3;
        }
        signal_ready(ready_path);
        return wait_for(release_path) ? 0 : 4;
    }
    if (mode == "recover-hold") {
        bool at_barrier = false;
        const auto result = hako::command::detail::recover_runtime_files(
            [&](const std::filesystem::path& path, std::error_code& error) {
                if (!at_barrier) {
                    at_barrier = true;
                    signal_ready(ready_path);
                    if (!wait_for(release_path)) {
                        error = std::make_error_code(std::errc::timed_out);
                        return false;
                    }
                }
                return std::filesystem::remove(path, error);
            });
        if (!at_barrier) {
            std::cerr << "recovery reached no deletion barrier: "
                      << result.status << " " << result.reason << std::endl;
            return 5;
        }
        if (!result.recovered) {
            std::cerr << "recovery failed: " << result.status << " "
                      << result.reason << std::endl;
            return 6;
        }
        return 0;
    }
    return 2;
}
