// Stage 5 benchmark server: polling style, no sleep. By default it does not register
// as an asset; with --register it registers as a polling asset and takes part in
// the time synchronization (asset_loop.hpp).
// Answers hako_srv_msgs/RenderCamera with a dummy rgb8 image of the size named by
// camera_name ("<width>x<height>") and logs per-request timestamps.
#include "asset_loop.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_RenderCameraRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_RenderCameraResponsePacket.hpp"
#include "bench_common.hpp"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace {
constexpr const char* kService = "HakoCamera/RenderCamera";
constexpr int kStateRunning = 2;
std::atomic_bool keep_running{true};
void stop_handler(int) { keep_running = false; }
}

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;
    const bool do_register = (argc == 4 && std::string(argv[3]) == "--register");
    if (argc != 3 && !do_register) {
        std::cerr << "Usage: " << argv[0] << " <service.json> <server.csv> [--register]\n";
        return 2;
    }
    std::signal(SIGINT, stop_handler);
    std::signal(SIGTERM, stop_handler);
    constexpr const char* kAsset = "BenchServer";
    if (hakoniwa_asset_init() != 0 || hakoniwa_simevent_init() != 0 ||
        (do_register && hakoniwa_asset_register_polling(kAsset) != 0) ||
        hako_asset_service_initialize(argv[1]) != 0) {
        std::cerr << "ERROR: initialization failed\n";
        return 1;
    }
    std::ofstream csv(argv[2]);
    csv << "seq,detect_ns,request_read_ns,image_filled_ns,encoded_ns,put_ns,server_time_usec\n";

    auto request_converter = hako::pdu::msgs::hako_srv_msgs::RenderCameraRequestPacket{};
    auto response_converter = hako::pdu::msgs::hako_srv_msgs::RenderCameraResponsePacket{};
    HakoCpp_RenderCameraResponsePacket response_packet{};
    int service_id = -1;
    auto create_server = [&]() {
        if (service_id < 0) {
            service_id = hako_asset_service_server_create(do_register ? kAsset : nullptr, kService);
            std::cout << "SERVER created id=" << service_id << "\n";
        }
        return service_id >= 0;
    };
    PollingAssetLoop loop{kAsset, 1000};

    while (keep_running) {
        if (do_register) {
            if (!loop.handle_event(create_server)) {
                return 1;
            }
            if (!loop.running()) {
                continue;
            }
            loop.advance_time();
        } else {
            const int state = hakoniwa_simevent_get_state();
            // RPC only while Running (see polling_server.cpp).
            if (!(state == kStateRunning && hakoniwa_asset_is_pdu_created())) {
                continue;
            }
            if (!create_server()) {
                std::cerr << "ERROR: server_create failed\n";
                return 1;
            }
        }
        if (hako_asset_service_server_poll(service_id) != HAKO_SERVICE_SERVER_API_REQUEST_IN) {
            continue;
        }
        const int64_t t_detect = bench_now_ns();
        char* request = nullptr;
        size_t request_len = 0;
        HakoCpp_RenderCameraRequestPacket request_packet{};
        if (hako_asset_service_server_get_request(service_id, &request, &request_len) != 0 ||
            !request_converter.pdu2cpp(request, request_packet)) {
            std::cerr << "ERROR: request read failed\n";
            return 1;
        }
        const int64_t t_read = bench_now_ns();

        unsigned width = 0, height = 0;
        std::sscanf(request_packet.body.camera_name.c_str(), "%ux%u", &width, &height);
        auto& image = response_packet.body.image;
        image.width = width;
        image.height = height;
        image.encoding = "rgb8";
        image.step = width * 3;
        image.data.assign(static_cast<size_t>(width) * height * 3,
                          static_cast<Hako_uint8>(request_packet.body.sim_time_usec & 0xff));
        response_packet.body.result_code = 0;
        response_packet.body.sim_time_usec = request_packet.body.sim_time_usec;
        const int64_t t_filled = bench_now_ns();

        char* response = nullptr;
        size_t response_len = 0;
        HakoCpp_RenderCameraResponsePacket header_only{};
        if (hako_asset_service_server_get_response_buffer(service_id, &response, &response_len,
                HAKO_SERVICE_API_STATUS_DONE, HAKO_SERVICE_API_RESULT_CODE_OK) != 0 ||
            !response_converter.pdu2cpp(response, header_only)) {
            std::cerr << "ERROR: response buffer failed\n";
            return 1;
        }
        response_packet.header = header_only.header;
        if (response_converter.cpp2pdu(response_packet, response, static_cast<int>(response_len)) < 0) {
            std::cerr << "ERROR: response encode failed\n";
            return 1;
        }
        const int64_t t_encoded = bench_now_ns();
        while (hako_asset_service_server_put_response(service_id, response, response_len) != 0) {
            if (!keep_running) return 0;
        }
        const int64_t t_put = bench_now_ns();
        csv << request_packet.body.sim_time_usec << ',' << t_detect << ',' << t_read << ','
            << t_filled << ',' << t_encoded << ',' << t_put << ',' << loop.local_time << '\n';
    }
    return 0;
}
