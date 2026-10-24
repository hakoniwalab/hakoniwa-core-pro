// Stage 5 benchmark client: polling asset, no sleep. Sends RenderCamera requests one
// at a time and logs the round trip and the decode time per request.
#include "hakoniwa_asset_polling.h"
#include "hako_srv_msgs/pdu_cpptype_conv_RenderCameraRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_RenderCameraResponsePacket.hpp"
#include "bench_common.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr const char* kAsset = "BenchClient";
constexpr const char* kService = "HakoCamera/RenderCamera";
constexpr const char* kClient = "BenchClient01";
constexpr hako_time_t kDeltaUsec = 1000;
constexpr int kStateRunning = 2;
constexpr int kEventStart = 1;
constexpr int kEventStop = 2;
constexpr int kWarmup = 20;
}

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <service.json> <width>x<height> <count> <client.csv>\n";
        return 2;
    }
    const std::string camera_name = argv[2];
    const int count = std::atoi(argv[3]);
    unsigned width = 0, height = 0;
    std::sscanf(camera_name.c_str(), "%ux%u", &width, &height);
    const size_t image_bytes = static_cast<size_t>(width) * height * 3;

    if (hakoniwa_asset_init() != 0 || hakoniwa_simevent_init() != 0 ||
        hakoniwa_asset_register_polling(kAsset) != 0 ||
        hako_asset_service_initialize(argv[1]) != 0) {
        std::cerr << "ERROR: initialization failed\n";
        return 1;
    }
    std::ofstream csv(argv[4]);
    csv << "seq,send_ns,response_in_ns,decoded_ns,bytes,client_time_usec,response_time_usec\n";

    auto request_converter = hako::pdu::msgs::hako_srv_msgs::RenderCameraRequestPacket{};
    auto response_converter = hako::pdu::msgs::hako_srv_msgs::RenderCameraResponsePacket{};
    HakoServiceHandleType handle{-1, -1};
    bool client_valid = false;
    hako_time_t local_time = 0;
    int sent = 0;

    for (;;) {
        const int event = hakoniwa_asset_get_event(kAsset);
        if (event == kEventStart) {
            // is_pdu_created() also loads the PDU data and the service tables into this process.
            if (!hakoniwa_asset_is_pdu_created()) {
                std::cerr << "ERROR: Start arrived before PDU creation\n";
                return 1;
            }
            if (hako_asset_service_client_create(kAsset, kService, kClient, &handle) != 0) {
                std::cerr << "ERROR: client_create failed\n";
                return 1;
            }
            client_valid = true;
            hakoniwa_asset_notify_write_pdu_done(kAsset);
            hakoniwa_asset_start_feedback_ok(kAsset);
        } else if (event == kEventStop) {
            client_valid = false;
            hakoniwa_asset_stop_feedback_ok(kAsset);
        }
        if (!(client_valid && hakoniwa_simevent_get_state() == kStateRunning)) {
            continue;
        }
        const hako_time_t world_time = hakoniwa_asset_get_worldtime();
        if (local_time <= world_time) {
            local_time += kDeltaUsec;
            hakoniwa_asset_notify_simtime(kAsset, local_time);
        }
        if (sent >= kWarmup + count) {
            break;
        }

        char* packet = nullptr;
        size_t packet_len = 0;
        HakoCpp_RenderCameraRequestPacket request{};
        if (hako_asset_service_client_get_request_buffer(&handle, &packet, &packet_len,
                HAKO_SERVICE_CLIENT_API_OPCODE_REQUEST, -1) != 0 ||
            !request_converter.pdu2cpp(packet, request)) return 1;
        request.body.camera_name = camera_name;
        request.body.sim_time_usec = static_cast<Hako_uint64>(sent);
        if (request_converter.cpp2pdu(request, packet, static_cast<int>(packet_len)) < 0) return 1;
        const int64_t t_send = bench_now_ns();
        if (hako_asset_service_client_call_request(&handle, packet, packet_len, 0) != 0) {
            std::cerr << "ERROR: request send failed\n";
            return 1;
        }
        // Keep the asset time moving while waiting: a server that answers inside its
        // simulation step can only run when the world time advances.
        const hako_time_t sent_time = local_time;
        int service_event = HAKO_SERVICE_CLIENT_API_EVENT_NONE;
        while ((service_event = hako_asset_service_client_poll(&handle)) == HAKO_SERVICE_CLIENT_API_EVENT_NONE) {
            if (local_time <= hakoniwa_asset_get_worldtime()) {
                local_time += kDeltaUsec;
                hakoniwa_asset_notify_simtime(kAsset, local_time);
            }
        }
        const int64_t t_in = bench_now_ns();
        if (service_event != HAKO_SERVICE_CLIENT_API_RESPONSE_IN) {
            std::cerr << "ERROR: unexpected service event " << service_event << "\n";
            return 1;
        }
        char* response = nullptr;
        size_t response_len = 0;
        HakoCpp_RenderCameraResponsePacket cpp_response{};
        if (hako_asset_service_client_get_response(&handle, &response, &response_len, 0) != 0 ||
            !response_converter.pdu2cpp(response, cpp_response)) return 1;
        const int64_t t_decoded = bench_now_ns();
        if (cpp_response.body.image.data.size() != image_bytes ||
            cpp_response.body.sim_time_usec != static_cast<Hako_uint64>(sent) ||
            (image_bytes > 0 && cpp_response.body.image.data[0] != static_cast<Hako_uint8>(sent & 0xff))) {
            std::cerr << "ERROR: bad response at " << sent << "\n";
            return 1;
        }
        if (sent >= kWarmup) {
            csv << sent << ',' << t_send << ',' << t_in << ',' << t_decoded << ',' << image_bytes << ',' << sent_time << ',' << local_time << '\n';
        }
        ++sent;
    }

    // Copy floor for the same size: one memcpy between two buffers.
    std::vector<char> src(image_bytes > 0 ? image_bytes : 1, 1), dst(src.size());
    int64_t best = INT64_MAX;
    for (int i = 0; i < 50; ++i) {
        const int64_t t0 = bench_now_ns();
        std::memcpy(dst.data(), src.data(), src.size());
        best = std::min(best, bench_now_ns() - t0);
    }
    std::cout << "MEMCPY bytes=" << image_bytes << " best_ns=" << best << " check=" << int(dst[0]) << "\n";
    std::cout << "CLIENT done count=" << count << "\n";
    return 0;
}
