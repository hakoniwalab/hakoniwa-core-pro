// RenderCamera client that moves the camera along a path: a polling asset (1 ms
// step, no sleep) that sends one request every <period-usec> of its simulation time
// with the next pose from a CSV (x,y,z,yaw_deg per line, ROS world frame), and keeps
// its time moving while it waits. Reports the round trip per frame and the RTF.
// --dump-dir=<dir> writes every 30th image as PPM (rgb8 only).
#include "hakoniwa_asset_polling.h"
#include "hako_srv_msgs/pdu_cpptype_conv_RenderCameraRequestPacket.hpp"
#include "hako_srv_msgs/pdu_cpptype_conv_RenderCameraResponsePacket.hpp"
#include "bench_common.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr const char* kAsset = "CameraPathClient";
constexpr const char* kService = "HakoCamera/RenderCamera";
constexpr const char* kClient = "CameraPathClient01";
constexpr hako_time_t kDeltaUsec = 1000;
constexpr int kStateRunning = 2;
constexpr int kEventStart = 1;
constexpr int kEventStop = 2;
// M_PI is not standard C++ (MSVC defines it only with _USE_MATH_DEFINES).
constexpr double kPi = 3.14159265358979323846;

struct Pose {
    double x, y, z, yaw_deg;
};

std::vector<Pose> load_path(const char* path)
{
    std::vector<Pose> poses;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        Pose p{};
        char comma = 0;
        if (ss >> p.x >> comma >> p.y >> comma >> p.z >> comma >> p.yaw_deg) {
            poses.push_back(p);
        }
    }
    return poses;
}

void dump_ppm(const std::string& path, const HakoCpp_RenderCameraResponsePacket& response)
{
    const auto& image = response.body.image;
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << image.width << " " << image.height << "\n255\n";
    out.write(reinterpret_cast<const char*>(image.data.data()), static_cast<std::streamsize>(image.data.size()));
}
}

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <service.json> <width>x<height> <path.csv> <period-usec> <client.csv> [--dump-dir=<dir>]\n";
        return 2;
    }
    const std::string camera_name = argv[2];
    const std::vector<Pose> poses = load_path(argv[3]);
    const hako_time_t period_usec = std::atoll(argv[4]);
    std::string dump_dir;
    if (argc == 7 && std::string(argv[6]).rfind("--dump-dir=", 0) == 0) {
        dump_dir = std::string(argv[6]).substr(11);
    }
    unsigned width = 0, height = 0;
    std::sscanf(camera_name.c_str(), "%ux%u", &width, &height);
    const size_t image_bytes = static_cast<size_t>(width) * height * 3;
    if (poses.empty() || period_usec < kDeltaUsec) {
        std::cerr << "ERROR: empty path or period shorter than one step\n";
        return 2;
    }

    if (hakoniwa_asset_init() != 0 || hakoniwa_simevent_init() != 0 ||
        hakoniwa_asset_register_polling(kAsset) != 0 ||
        hako_asset_service_initialize(argv[1]) != 0) {
        std::cerr << "ERROR: initialization failed\n";
        return 1;
    }
    std::ofstream csv(argv[5]);
    csv << "frame,send_ns,response_in_ns,request_time_usec,response_time_usec\n";

    auto request_converter = hako::pdu::msgs::hako_srv_msgs::RenderCameraRequestPacket{};
    auto response_converter = hako::pdu::msgs::hako_srv_msgs::RenderCameraResponsePacket{};
    HakoServiceHandleType handle{-1, -1};
    bool client_valid = false;
    hako_time_t local_time = 0;
    hako_time_t next_request_time = 0;
    size_t frame = 0;
    int64_t start_ns = 0;

    auto advance_time = [&]() {
        if (local_time <= hakoniwa_asset_get_worldtime()) {
            local_time += kDeltaUsec;
            hakoniwa_asset_notify_simtime(kAsset, local_time);
        }
    };

    for (;;) {
        const int event = hakoniwa_asset_get_event(kAsset);
        if (event == kEventStart) {
            // is_pdu_created() also loads the PDU data and the service tables into this process.
            if (!hakoniwa_asset_is_pdu_created() ||
                hako_asset_service_client_create(kAsset, kService, kClient, &handle) != 0) {
                std::cerr << "ERROR: client_create failed\n";
                return 1;
            }
            client_valid = true;
            hakoniwa_asset_notify_write_pdu_done(kAsset);
            hakoniwa_asset_start_feedback_ok(kAsset);
            continue;
        }
        if (event == kEventStop) {
            hakoniwa_asset_stop_feedback_ok(kAsset);
            break;
        }
        if (!client_valid || hakoniwa_simevent_get_state() != kStateRunning) {
            continue;
        }
        advance_time();
        if (frame >= poses.size()) {
            break;
        }
        if (local_time < next_request_time) {
            continue;
        }

        const Pose& pose = poses[frame];
        char* packet = nullptr;
        size_t packet_len = 0;
        HakoCpp_RenderCameraRequestPacket request{};
        if (hako_asset_service_client_get_request_buffer(&handle, &packet, &packet_len,
                HAKO_SERVICE_CLIENT_API_OPCODE_REQUEST, -1) != 0 ||
            !request_converter.pdu2cpp(packet, request)) return 1;
        const double yaw = pose.yaw_deg * kPi / 180.0;
        request.body.camera_name = camera_name;
        request.body.sim_time_usec = static_cast<Hako_uint64>(local_time);
        request.body.camera_pose.position.x = pose.x;
        request.body.camera_pose.position.y = pose.y;
        request.body.camera_pose.position.z = pose.z;
        request.body.camera_pose.orientation.z = std::sin(yaw * 0.5);
        request.body.camera_pose.orientation.w = std::cos(yaw * 0.5);
        if (request_converter.cpp2pdu(request, packet, static_cast<int>(packet_len)) < 0) return 1;
        const int64_t t_send = bench_now_ns();
        if (start_ns == 0) start_ns = t_send;
        const hako_time_t request_time = local_time;
        if (hako_asset_service_client_call_request(&handle, packet, packet_len, 0) != 0) {
            std::cerr << "ERROR: request send failed\n";
            return 1;
        }
        int service_event = HAKO_SERVICE_CLIENT_API_EVENT_NONE;
        while ((service_event = hako_asset_service_client_poll(&handle)) == HAKO_SERVICE_CLIENT_API_EVENT_NONE) {
            advance_time();
        }
        const int64_t t_in = bench_now_ns();
        char* response = nullptr;
        size_t response_len = 0;
        HakoCpp_RenderCameraResponsePacket cpp_response{};
        if (service_event != HAKO_SERVICE_CLIENT_API_RESPONSE_IN ||
            hako_asset_service_client_get_response(&handle, &response, &response_len, 0) != 0 ||
            !response_converter.pdu2cpp(response, cpp_response) ||
            cpp_response.body.sim_time_usec != static_cast<Hako_uint64>(request_time) ||
            cpp_response.body.image.data.size() != image_bytes) {
            std::cerr << "ERROR: bad response at frame " << frame << "\n";
            return 1;
        }
        csv << frame << ',' << t_send << ',' << t_in << ',' << request_time << ',' << local_time << '\n';
        if (!dump_dir.empty() && frame % 30 == 0) {
            char name[64];
            std::snprintf(name, sizeof(name), "/frame_%04zu.ppm", frame);
            dump_ppm(dump_dir + name, cpp_response);
        }
        ++frame;
        next_request_time = request_time + period_usec;
    }
    const double wall_sec = (bench_now_ns() - start_ns) / 1e9;
    std::cout << "CLIENT done frames=" << frame << " sim_sec=" << local_time / 1e6
              << " wall_sec=" << wall_sec << " RTF=" << (local_time / 1e6) / wall_sec << "\n";
    return 0;
}
