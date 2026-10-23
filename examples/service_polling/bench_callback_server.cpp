// Stage 5 benchmark server in the callback style: hako_asset_register, the service
// is created in on_initialize and polled in on_simulation_step (one call per step).
#include "hako_asset.h"
#include "hako_asset_service.h"
#include "hako_srv_msgs/pdu_cpptype_conv_RenderCameraRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_RenderCameraResponsePacket.hpp"
#include "bench_common.hpp"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <thread>

namespace {
constexpr const char* kAsset = "BenchCallbackServer";
constexpr const char* kService = "HakoCamera/RenderCamera";
constexpr hako_time_t kDeltaUsec = 1000;
int service_id = -1;
hako_time_t step_time_usec = 0;
std::ofstream csv;
HakoCpp_RenderCameraResponsePacket response_packet{};

int on_initialize(hako_asset_context_t*)
{
    service_id = hako_asset_service_server_create(kAsset, kService);
    step_time_usec = 0;
    std::cout << "SERVER on_initialize created id=" << service_id << "\n";
    return service_id < 0 ? 1 : 0;
}

int on_simulation_step(hako_asset_context_t*)
{
    step_time_usec += kDeltaUsec;
    if (service_id < 0 || hako_asset_service_server_poll(service_id) != HAKO_SERVICE_SERVER_API_REQUEST_IN) {
        return 0;
    }
    const int64_t t_detect = bench_now_ns();
    char* request = nullptr;
    size_t request_len = 0;
    HakoCpp_RenderCameraRequestPacket request_packet{};
    if (hako_asset_service_server_get_request(service_id, &request, &request_len) != 0 ||
        !hako::pdu::msgs::hako_srv_msgs::RenderCameraRequestPacket{}.pdu2cpp(request, request_packet)) {
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
    auto converter = hako::pdu::msgs::hako_srv_msgs::RenderCameraResponsePacket{};
    HakoCpp_RenderCameraResponsePacket header_only{};
    if (hako_asset_service_server_get_response_buffer(service_id, &response, &response_len,
            HAKO_SERVICE_API_STATUS_DONE, HAKO_SERVICE_API_RESULT_CODE_OK) != 0 ||
        !converter.pdu2cpp(response, header_only)) {
        return 1;
    }
    response_packet.header = header_only.header;
    if (converter.cpp2pdu(response_packet, response, static_cast<int>(response_len)) < 0) return 1;
    const int64_t t_encoded = bench_now_ns();
    while (hako_asset_service_server_put_response(service_id, response, response_len) != 0) {
    }
    const int64_t t_put = bench_now_ns();
    csv << request_packet.body.sim_time_usec << ',' << t_detect << ',' << t_read << ','
        << t_filled << ',' << t_encoded << ',' << t_put << ',' << step_time_usec << std::endl;  // killed, not exited
    return 0;
}

int on_reset(hako_asset_context_t*)
{
    step_time_usec = 0;
    return 0;
}
}

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <pdu_config.json> <service.json> <server.csv>\n";
        return 2;
    }
    csv.open(argv[3]);
    csv << "seq,detect_ns,request_read_ns,image_filled_ns,encoded_ns,put_ns,server_time_usec\n";
    static hako_asset_callbacks_t callbacks{};
    callbacks.on_initialize = on_initialize;
    callbacks.on_simulation_step = on_simulation_step;
    callbacks.on_reset = on_reset;
    if (hako_asset_register(kAsset, argv[1], &callbacks, kDeltaUsec, HAKO_ASSET_MODEL_PLANT) != 0 ||
        hako_asset_service_initialize(argv[2]) != 0) {
        std::cerr << "ERROR: register/service_initialize failed\n";
        return 1;
    }
    for (;;) {
        const int err = hako_asset_start_no_wait(nullptr);
        csv.flush();
        if (err == EINVAL) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else if (err != EINTR) {
            return 1;
        }
    }
}
