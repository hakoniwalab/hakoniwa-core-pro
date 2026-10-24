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
    // --late-reply also prepares a marked response (sum=999) for that request and writes
    // it after the next start, like a worker that finishes after the run was restarted.
    const std::string option = (argc == 3) ? argv[2] : "";
    const bool late_reply = (option == "--late-reply");
    const bool hold_first = late_reply || (option == "--hold-first-request");
    if (argc != 2 && !hold_first) {
        std::cerr << "Usage: " << argv[0] << " <service.json> [--hold-first-request|--late-reply]\n";
        return 2;
    }
    bool held = false;
    char* late_response = nullptr;
    size_t late_response_len = 0;
    int start_count = 0;
    bool late_due = false;
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
        if (++start_count > 1 && late_response != nullptr) {
            late_due = true;
        }
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
        if (late_due) {
            late_due = false;
            const int rc = hako_asset_service_server_put_response(service_id, late_response, late_response_len);
            std::cout << "SERVER late reply of the previous run: put_response returned " << rc << "\n";
            late_response = nullptr;
        }
        if (hako_asset_service_server_poll(service_id) != HAKO_SERVICE_SERVER_API_REQUEST_IN) {
            continue;
        }
        if (hold_first && !held) {
            held = true;
            std::cout << "SERVER holding the first request (DOING)\n";
            if (late_reply) {
                HakoCpp_AddTwoIntsResponsePacket marked{};
                if (hako_asset_service_server_get_response_buffer(service_id, &late_response, &late_response_len,
                        HAKO_SERVICE_API_STATUS_DONE, HAKO_SERVICE_API_RESULT_CODE_OK) != 0 ||
                    !response_converter.pdu2cpp(late_response, marked)) {
                    return 1;
                }
                marked.body.sum = 999;
                if (response_converter.cpp2pdu(marked, late_response, static_cast<int>(late_response_len)) < 0) {
                    return 1;
                }
            }
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
