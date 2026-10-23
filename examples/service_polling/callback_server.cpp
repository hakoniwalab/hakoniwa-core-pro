// Comparison only: the same Service/Add server written in the callback style
// (hako_asset_register + hako_asset_start_no_wait), restarting start like drone-pro.
#include "hako_asset.h"
#include "hako_asset_service.h"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsResponsePacket.hpp"

#include <cerrno>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {
constexpr const char* kAsset = "CallbackServer";
constexpr const char* kService = "Service/Add";
int service_id = -1;
bool pending_response = false;
bool hold_first = false;
bool held = false;
bool late_reply = false;
char* late_response = nullptr;
size_t late_response_len = 0;
int start_count = 0;
bool late_due = false;
char* response = nullptr;
size_t response_len = 0;

int on_initialize(hako_asset_context_t*)
{
    if (++start_count > 1 && late_response != nullptr) {
        late_due = true;
    }
    service_id = hako_asset_service_server_create(kAsset, kService);
    pending_response = false;
    std::cout << "SERVER on_initialize created id=" << service_id << "\n";
    return service_id < 0 ? 1 : 0;
}

int on_simulation_step(hako_asset_context_t*)
{
    if (service_id < 0) return 0;
    if (late_due) {
        late_due = false;
        const int rc = hako_asset_service_server_put_response(service_id, late_response, late_response_len);
        std::cout << "SERVER late reply of the previous run: put_response returned " << rc << "\n";
        late_response = nullptr;
    }
    if (pending_response) {
        if (hako_asset_service_server_put_response(service_id, response, response_len) == 0) {
            pending_response = false;
            std::cout << "SERVER response sent\n";
        }
        return 0;
    }
    if (hako_asset_service_server_poll(service_id) != HAKO_SERVICE_SERVER_API_REQUEST_IN) return 0;
    if (hold_first && !held) {
        held = true;
        std::cout << "SERVER holding the first request (DOING)\n";
        if (late_reply) {
            auto converter = hako::pdu::msgs::hako_srv_msgs::AddTwoIntsResponsePacket{};
            HakoCpp_AddTwoIntsResponsePacket marked{};
            if (hako_asset_service_server_get_response_buffer(service_id, &late_response, &late_response_len,
                    HAKO_SERVICE_API_STATUS_DONE, HAKO_SERVICE_API_RESULT_CODE_OK) != 0 ||
                !converter.pdu2cpp(late_response, marked)) {
                return 1;
            }
            marked.body.sum = 999;
            if (converter.cpp2pdu(marked, late_response, static_cast<int>(late_response_len)) < 0) {
                return 1;
            }
        }
        return 0;
    }
    char* request = nullptr;
    size_t request_len = 0;
    HakoCpp_AddTwoIntsRequestPacket cpp_request{};
    HakoCpp_AddTwoIntsResponsePacket cpp_response{};
    auto converter = hako::pdu::msgs::hako_srv_msgs::AddTwoIntsResponsePacket{};
    if (hako_asset_service_server_get_request(service_id, &request, &request_len) != 0 ||
        !hako::pdu::msgs::hako_srv_msgs::AddTwoIntsRequestPacket{}.pdu2cpp(request, cpp_request) ||
        hako_asset_service_server_get_response_buffer(service_id, &response, &response_len,
            HAKO_SERVICE_API_STATUS_DONE, HAKO_SERVICE_API_RESULT_CODE_OK) != 0 ||
        !converter.pdu2cpp(response, cpp_response)) {
        std::cerr << "ERROR: request/response handling failed\n";
        return 1;
    }
    cpp_response.body.sum = cpp_request.body.a + cpp_request.body.b;
    if (converter.cpp2pdu(cpp_response, response, static_cast<int>(response_len)) < 0) return 1;
    std::cout << "SERVER request a=" << cpp_request.body.a << " b=" << cpp_request.body.b
              << " sum=" << cpp_response.body.sum << "\n";
    pending_response = true;
    return 0;
}

int on_reset(hako_asset_context_t*)
{
    std::cout << "SERVER on_reset\n";
    return 0;
}
}

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;  // keep logs in order when redirected
    const std::string option = (argc == 4) ? argv[3] : "";
    late_reply = (option == "--late-reply");
    hold_first = late_reply || (option == "--hold-first-request");
    if (argc != 3 && !hold_first) {
        std::cerr << "Usage: " << argv[0] << " <pdu_config.json> <service.json> [--hold-first-request|--late-reply]\n";
        return 2;
    }
    static hako_asset_callbacks_t callbacks{};
    callbacks.on_initialize = on_initialize;
    callbacks.on_simulation_step = on_simulation_step;
    callbacks.on_reset = on_reset;
    if (hako_asset_register(kAsset, argv[1], &callbacks, 1000, HAKO_ASSET_MODEL_PLANT) != 0 ||
        hako_asset_service_initialize(argv[2]) != 0) {
        std::cerr << "ERROR: register/service_initialize failed\n";
        return 1;
    }
    for (;;) {
        const int err = hako_asset_start_no_wait(nullptr);
        std::cout << "SERVER hako_asset_start_no_wait returned " << err << "\n";
        if (err == EINVAL) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else if (err != EINTR) {
            return 1;
        }
    }
}
