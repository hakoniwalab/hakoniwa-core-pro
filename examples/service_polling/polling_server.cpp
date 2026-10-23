#include "hakoniwa_asset_polling.h"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsResponsePacket.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>

namespace {
constexpr const char* kService = "Service/Add";
constexpr int kStateRunnable = 1;
constexpr int kStateRunning = 2;
std::atomic_bool keep_running{true};

void stop_handler(int) { keep_running = false; }

const char* state_name(int state)
{
    static const char* names[] = {"Stopped", "Runnable", "Running", "Stopping",
                                  "Resetting", "Error", "Terminated"};
    return state >= 0 && state <= 6 ? names[state] : "Unknown";
}
}

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;  // keep logs in order when redirected
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <service.json>\n";
        return 2;
    }
    std::signal(SIGINT, stop_handler);
    std::signal(SIGTERM, stop_handler);

    if (hakoniwa_asset_init() != 0 || hakoniwa_simevent_init() != 0) {
        std::cerr << "ERROR: polling API initialization failed\n";
        return 1;
    }
    if (hako_asset_service_initialize(argv[1]) != 0) {
        std::cerr << "ERROR: service declaration failed in Stopped\n";
        return 1;
    }
    std::cout << "SERVER service declared in Stopped\n";

    int last_state = -1;
    int service_id = -1;
    bool pending_response = false;
    char* response = nullptr;
    size_t response_len = 0;

    while (keep_running) {
        const int state = hakoniwa_simevent_get_state();
        if (state != last_state) {
            std::cout << "SERVER state " << state_name(last_state) << " -> "
                      << state_name(state) << "\n";
            last_state = state;
        }

        const bool active = (state == kStateRunnable || state == kStateRunning);
        if (active && hakoniwa_asset_is_pdu_created()) {
            if (service_id < 0) {
                service_id = hako_asset_service_server_create(nullptr, kService);
                if (service_id < 0) {
                    std::cerr << "ERROR: server_create failed\n";
                    keep_running = false;
                } else {
                    std::cout << "SERVER created id=" << service_id << "\n";
                }
            }
            if (service_id >= 0 && pending_response) {
                if (hako_asset_service_server_put_response(service_id, response, response_len) == 0) {
                    pending_response = false;
                    std::cout << "SERVER response sent\n";
                }
            } else if (service_id >= 0) {
                const int event = hako_asset_service_server_poll(service_id);
                if (event == HAKO_SERVICE_SERVER_API_REQUEST_IN) {
                    char* request = nullptr;
                    size_t request_len = 0;
                    HakoCpp_AddTwoIntsRequestPacket cpp_request{};
                    if (hako_asset_service_server_get_request(service_id, &request, &request_len) != 0 ||
                        !hako::pdu::msgs::hako_srv_msgs::AddTwoIntsRequestPacket{}.pdu2cpp(request, cpp_request) ||
                        hako_asset_service_server_get_response_buffer(
                            service_id, &response, &response_len,
                            HAKO_SERVICE_API_STATUS_DONE, HAKO_SERVICE_API_RESULT_CODE_OK) != 0) {
                        std::cerr << "ERROR: request/response buffer handling failed\n";
                        keep_running = false;
                    } else {
                        HakoCpp_AddTwoIntsResponsePacket cpp_response{};
                        auto converter = hako::pdu::msgs::hako_srv_msgs::AddTwoIntsResponsePacket{};
                        if (!converter.pdu2cpp(response, cpp_response)) {
                            std::cerr << "ERROR: response header conversion failed\n";
                            keep_running = false;
                        } else {
                            cpp_response.body.sum = cpp_request.body.a + cpp_request.body.b;
                            if (converter.cpp2pdu(cpp_response, response, static_cast<int>(response_len)) < 0) {
                                std::cerr << "ERROR: response body conversion failed\n";
                                keep_running = false;
                            } else {
                                std::cout << "SERVER request a=" << cpp_request.body.a
                                          << " b=" << cpp_request.body.b
                                          << " sum=" << cpp_response.body.sum << "\n";
                                pending_response = true;
                            }
                        }
                    }
                } else if (event < 0) {
                    std::cerr << "ERROR: server poll failed\n";
                    keep_running = false;
                }
            }
        } else if (service_id >= 0) {
            std::cout << "SERVER invalidated id=" << service_id << "\n";
            service_id = -1;
            pending_response = false;
            response = nullptr;
            response_len = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
