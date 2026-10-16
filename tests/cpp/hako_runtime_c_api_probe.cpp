#include "hako_asset.h"
#include "hako_asset_service.h"
#include "hako_runtime_error.h"

#include <cerrno>
#include <iostream>
#include <string>

namespace {

bool expect_abort(const char *name, int result, int expected_result)
{
    if (result != expected_result ||
        hako_runtime_last_error_status() != HAKO_RUNTIME_ERROR_ABORT ||
        std::string(hako_runtime_last_error()).empty()) {
        std::cerr << name << " was not contained as ABORT: result=" << result
                  << " status="
                  << hako_runtime_error_status_name(
                         hako_runtime_last_error_status())
                  << " message=" << hako_runtime_last_error() << std::endl;
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (hako_initialize_for_external() != 0) {
        std::cerr << "healthy external initialization failed" << std::endl;
        return 3;
    }
    std::cout << "READY" << std::endl;
    std::string command;
    if (!std::getline(std::cin, command) || command != "CHECK") {
        return 4;
    }

    if (!expect_abort(
            "hako_asset_pdu_create",
            hako_asset_pdu_create("NativeRuntimeProbe", 78, 8), EIO)) {
        return 5;
    }
    if (!expect_abort(
            "hako_asset_service_initialize",
            hako_asset_service_initialize(argv[1]), -1)) {
        return 6;
    }
    std::cout << "ABORT_OK" << std::endl;
    return 0;
}

