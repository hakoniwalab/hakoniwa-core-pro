#include "hakoniwa_asset_polling.h"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_AddTwoIntsResponsePacket.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <thread>

namespace {
constexpr const char* kAsset = "PollingClient";
constexpr const char* kService = "Service/Add";
constexpr const char* kClient = "Client01";
constexpr hako_time_t kDeltaUsec = 1000;
constexpr int kStateRunning = 2;
constexpr int kEventNone = 0;
constexpr int kEventStart = 1;
constexpr int kEventStop = 2;
constexpr int kEventReset = 3;
constexpr int kEventError = 4;

const char* event_name(int event)
{
    static const char* names[] = {"None", "Start", "Stop", "Reset", "Error"};
    return event >= 0 && event <= 4 ? names[event] : "Unknown";
}
}

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;  // keep logs in order when redirected
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <service.json> <request-count>\n";
        return 2;
    }
    char* end = nullptr;
    const long requested = std::strtol(argv[2], &end, 10);
    if (*argv[2] == '\0' || *end != '\0' || requested <= 0 ||
        requested > std::numeric_limits<int>::max()) {
        std::cerr << "ERROR: request-count must be a positive integer\n";
        return 2;
    }

    if (hakoniwa_asset_init() != 0 || hakoniwa_simevent_init() != 0 ||
        hakoniwa_asset_register_polling(kAsset) != 0) {
        std::cerr << "ERROR: polling asset initialization failed\n";
        return 1;
    }
    if (hako_asset_service_initialize(argv[1]) != 0) {
        std::cerr << "ERROR: service declaration failed in Stopped\n";
        return 1;
    }
    std::cout << "CLIENT registered; service declared in Stopped\n";

    HakoServiceHandleType handle{-1, -1};
    bool client_valid = false;
    bool waiting = false;
    int completed = 0;
    hako_time_t local_time = 0;
    std::chrono::steady_clock::time_point sent_at{};
    double total_usec = 0.0;
    double max_usec = 0.0;

    for (;;) {
        const int event = hakoniwa_asset_get_event(kAsset);
        if (event != kEventNone) {
            std::cout << "CLIENT event " << event_name(event) << "\n";
        }
        if (event == kEventStart) {
            if (!hakoniwa_asset_is_pdu_created()) {
                std::cerr << "ERROR: Start arrived before PDU creation\n";
                return 1;
            }
            if (hako_asset_service_client_create(kAsset, kService, kClient, &handle) != 0) {
                std::cerr << "ERROR: client_create failed\n";
                return 1;
            }
            client_valid = true;
            waiting = false;
            local_time = 0;
            completed = 0;
            total_usec = 0.0;
            max_usec = 0.0;
            std::cout << "CLIENT created service_id=" << handle.service_id
                      << " client_id=" << handle.client_id << "\n";
            hakoniwa_asset_notify_write_pdu_done(kAsset);
            if (hakoniwa_asset_start_feedback_ok(kAsset) != 0) return 1;
        } else if (event == kEventStop) {
            client_valid = false;
            waiting = false;
            std::cout << "CLIENT invalidated\n";
            if (hakoniwa_asset_stop_feedback_ok(kAsset) != 0) return 1;
        } else if (event == kEventReset) {
            client_valid = false;
            waiting = false;
            local_time = 0;
            completed = 0;
            total_usec = 0.0;
            max_usec = 0.0;
            std::cout << "CLIENT invalidated by reset\n";
            if (hakoniwa_asset_reset_feedback_ok(kAsset) != 0) return 1;
        } else if (event == kEventError) {
            return 1;
        }

        if (client_valid && hakoniwa_simevent_get_state() == kStateRunning &&
            hakoniwa_asset_is_pdu_created()) {
            if (!waiting && completed < requested) {
                char* packet = nullptr;
                size_t packet_len = 0;
                if (hako_asset_service_client_get_request_buffer(
                        &handle, &packet, &packet_len,
                        HAKO_SERVICE_CLIENT_API_OPCODE_REQUEST, -1) != 0) return 1;
                HakoCpp_AddTwoIntsRequestPacket cpp_packet{};
                auto converter = hako::pdu::msgs::hako_srv_msgs::AddTwoIntsRequestPacket{};
                if (!converter.pdu2cpp(packet, cpp_packet)) return 1;
                cpp_packet.body.a = completed + 1;
                cpp_packet.body.b = (completed + 1) * 2;
                if (converter.cpp2pdu(cpp_packet, packet, static_cast<int>(packet_len)) < 0 ||
                    hako_asset_service_client_call_request(&handle, packet, packet_len, 5000) != 0) {
                    std::cerr << "ERROR: request send failed at " << completed << "\n";
                    return 1;
                }
                sent_at = std::chrono::steady_clock::now();
                waiting = true;
            } else if (waiting) {
                const int service_event = hako_asset_service_client_poll(&handle);
                if (service_event == HAKO_SERVICE_CLIENT_API_RESPONSE_IN) {
                    char* packet = nullptr;
                    size_t packet_len = 0;
                    HakoCpp_AddTwoIntsResponsePacket cpp_response{};
                    if (hako_asset_service_client_get_response(&handle, &packet, &packet_len, 0) != 0 ||
                        !hako::pdu::msgs::hako_srv_msgs::AddTwoIntsResponsePacket{}.pdu2cpp(packet, cpp_response)) return 1;
                    const long long expected = static_cast<long long>(completed + 1) * 3;
                    if (cpp_response.body.sum != expected) {
                        std::cerr << "ERROR: bad sum expected=" << expected
                                  << " actual=" << cpp_response.body.sum << "\n";
                        return 1;
                    }
                    const double usec = std::chrono::duration<double, std::micro>(
                        std::chrono::steady_clock::now() - sent_at).count();
                    total_usec += usec;
                    if (usec > max_usec) max_usec = usec;
                    ++completed;
                    waiting = false;
                    std::cout << "CLIENT response " << completed << "/" << requested
                              << " sum=" << cpp_response.body.sum
                              << " rtt_us=" << usec << "\n";
                    if (completed == requested) {
                        std::cout << "RESULT count=" << completed
                                  << " average_rtt_us=" << total_usec / completed
                                  << " max_rtt_us=" << max_usec << "\n";
                    }
                } else if (service_event == HAKO_SERVICE_CLIENT_API_REQUEST_TIMEOUT || service_event < 0) {
                    std::cerr << "ERROR: service request timeout/poll failure\n";
                    return 1;
                }
            }

            const hako_time_t world_time = hakoniwa_asset_get_worldtime();
            if (local_time <= world_time) {
                local_time += kDeltaUsec;
                hakoniwa_asset_notify_simtime(kAsset, local_time);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
