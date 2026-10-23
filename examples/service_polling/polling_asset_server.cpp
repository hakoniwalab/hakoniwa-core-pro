// Service/Add server that registers as a polling asset and takes part in the
// time synchronization. No callbacks, no sleep.
#include "asset_loop.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsResponsePacket.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

namespace {
constexpr const char* kAsset = "PollingAssetServer";
constexpr const char* kService = "Service/Add";
std::atomic_bool keep_running{true};
void stop_handler(int) { keep_running = false; }
}

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;  // keep logs in order when redirected
    // --hold-first-request leaves the first request unanswered (the server stays DOING),
    // to test a stop/reset/start while a request is in progress.
    const bool hold_first = (argc == 3 && std::string(argv[2]) == "--hold-first-request");
    if (argc != 2 && !hold_first) {
        std::cerr << "Usage: " << argv[0] << " <service.json> [--hold-first-request]\n";
        return 2;
    }
    bool held = false;
    std::signal(SIGINT, stop_handler);
    std::signal(SIGTERM, stop_handler);
    if (hakoniwa_asset_init() != 0 || hakoniwa_simevent_init() != 0 ||
        hakoniwa_asset_register_polling(kAsset) != 0 ||
        hako_asset_service_initialize(argv[1]) != 0) {
        std::cerr << "ERROR: initialization failed\n";
        return 1;
    }
    std::cout << "SERVER registered; service declared in Stopped\n";

    PollingAssetLoop loop{kAsset, 1000};
    int service_id = -1;
    auto create_server = [&]() {
        if (service_id < 0) {
            service_id = hako_asset_service_server_create(kAsset, kService);
            std::cout << "SERVER created id=" << service_id << "\n";
        }
        return service_id >= 0;
    };
    auto request_converter = hako::pdu::msgs::hako_srv_msgs::AddTwoIntsRequestPacket{};
    auto response_converter = hako::pdu::msgs::hako_srv_msgs::AddTwoIntsResponsePacket{};

    while (keep_running) {
        if (!loop.handle_event(create_server)) {
            return 1;
        }
        if (!loop.running()) {
            continue;
        }
        loop.advance_time();
        if (hako_asset_service_server_poll(service_id) != HAKO_SERVICE_SERVER_API_REQUEST_IN) {
            continue;
        }
        if (hold_first && !held) {
            held = true;
            std::cout << "SERVER holding the first request (DOING)\n";
            continue;
        }
        char* request = nullptr;
        size_t request_len = 0;
        char* response = nullptr;
        size_t response_len = 0;
        HakoCpp_AddTwoIntsRequestPacket cpp_request{};
        HakoCpp_AddTwoIntsResponsePacket cpp_response{};
        if (hako_asset_service_server_get_request(service_id, &request, &request_len) != 0 ||
            !request_converter.pdu2cpp(request, cpp_request) ||
            hako_asset_service_server_get_response_buffer(service_id, &response, &response_len,
                HAKO_SERVICE_API_STATUS_DONE, HAKO_SERVICE_API_RESULT_CODE_OK) != 0 ||
            !response_converter.pdu2cpp(response, cpp_response)) {
            std::cerr << "ERROR: request/response handling failed\n";
            return 1;
        }
        cpp_response.body.sum = cpp_request.body.a + cpp_request.body.b;
        if (response_converter.cpp2pdu(cpp_response, response, static_cast<int>(response_len)) < 0) {
            return 1;
        }
        while (hako_asset_service_server_put_response(service_id, response, response_len) != 0) {
            if (!keep_running) return 0;
        }
        std::cout << "SERVER request a=" << cpp_request.body.a << " b=" << cpp_request.body.b
                  << " sum=" << cpp_response.body.sum << " server_time_usec=" << loop.local_time << "\n";
    }
    return 0;
}
