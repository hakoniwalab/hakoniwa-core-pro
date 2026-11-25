#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "hako_service_protocol_client.hpp"
#include "hako_service_protocol_server.hpp"

namespace {

using namespace hako::service;

constexpr char kService[] = "Service/InterimReply";
constexpr char kClient[] = "client-a";
constexpr char kOtherClient[] = "client-b";
constexpr int kPacketSize = 512;
using RequestConverter = hako::pdu::msgs::hako_srv_msgs::ServiceRequestHeader;
using ResponseConverter = hako::pdu::msgs::hako_srv_msgs::ServiceResponseHeader;

class ServerTransport final : public IHakoServiceServer {
public:
    struct Response {
        int client;
        HakoCpp_ServiceResponseHeader header;
    };

    std::vector<Response> responses;
    int rotations = 0;
    int completions = 0;

    bool queue_request(
        std::uint32_t id, HakoServiceOperationCodeType operation = HAKO_SERVICE_OPERATION_CODE_REQUEST,
        int progress_interval_ms = -1)
    {
        HakoCpp_ServiceRequestHeader header {};
        header.request_id = id;
        header.service_name = kService;
        header.client_name = kClient;
        header.opcode = operation;
        header.status_poll_interval_msec = progress_interval_ms;
        pending_ = RequestConverter {}.cpp2pdu(header, request_.data(), kPacketSize) > 0;
        return pending_;
    }

    bool initialize(const char*, const char*) override { return true; }
    char* recv_request(int client_id) override
    {
        return client_id == 0 && std::exchange(pending_, false) ? request_.data() : nullptr;
    }
    bool send_response(int client_id, void* packet, int) override
    {
        HakoCpp_ServiceResponseHeader header {};
        if (!ResponseConverter {}.pdu2cpp(static_cast<char*>(packet), header)) {
            return false;
        }
        responses.push_back({client_id, std::move(header)});
        return true;
    }
    int get_current_client_id() override { return current_; }
    void next_client() override
    {
        ++rotations;
        current_ = (current_ + 1) % 2;
    }
    void reset_run_state() override
    {
        current_ = 0;
        states_.fill(HAKO_SERVICE_SERVER_STATE_IDLE);
    }
    HakoServiceServerStateType get_state() override { return states_[current_]; }

    int get_service_id() override { return 0; }
    int get_asset_id() override { return 0; }
    void* get_temp_request_buffer() override { return request_.data(); }
    void* get_temp_response_buffer() override { return response_.data(); }
    bool event_start_service(int client_id) override
    {
        states_[client_id] = HAKO_SERVICE_SERVER_STATE_DOING;
        return true;
    }
    bool event_done_service(int client_id) override
    {
        ++completions;
        states_[client_id] = HAKO_SERVICE_SERVER_STATE_IDLE;
        return true;
    }
    bool event_cancel_service(int client_id) override
    {
        states_[client_id] = HAKO_SERVICE_SERVER_STATE_CANCELING;
        return true;
    }
    int get_request_pdu_size() override { return kPacketSize; }
    int get_response_pdu_size() override { return kPacketSize; }
    std::string get_service_name() override { return kService; }
    std::string get_client_name(int client_id) override { return client_id == 0 ? kClient : kOtherClient; }
    bool is_exist_client(std::string name) override { return name == kClient || name == kOtherClient; }
    int get_current_request_channel_id(int client_id) override { return client_id * 2; }
    int get_current_response_channel_id(int client_id) override { return client_id * 2 + 1; }

private:
    alignas(std::max_align_t) std::array<char, kPacketSize> request_ {};
    alignas(std::max_align_t) std::array<char, kPacketSize> response_ {};
    std::array<HakoServiceServerStateType, 2> states_ {HAKO_SERVICE_SERVER_STATE_IDLE,
                                                   HAKO_SERVICE_SERVER_STATE_IDLE};
    int current_ = 0;
    bool pending_ = false;
};

class ClientTransport final : public IHakoServiceClient {
public:
    int sends = 0;

    bool queue_response(
        std::uint32_t id, HakoServiceStatusType status, HakoServiceResultCodeType result,
        std::uint8_t progress)
    {
        HakoCpp_ServiceResponseHeader header {};
        header.request_id = id;
        header.service_name = kService;
        header.client_name = kClient;
        header.status = status;
        header.result_code = result;
        header.processing_percentage = progress;
        pending_ = ResponseConverter {}.cpp2pdu(header, response_.data(), kPacketSize) > 0;
        return pending_;
    }

    bool initialize(const char*, const char*, const char*) override { return true; }
    char* recv_response() override
    {
        return std::exchange(pending_, false) ? response_.data() : nullptr;
    }
    bool send_request(void*, int) override
    {
        ++sends;
        return true;
    }
    HakoServiceClientStateType get_state() override { return state_; }
    void reset_run_state() override { state_ = HAKO_SERVICE_CLIENT_STATE_IDLE; }
    int get_service_id() override { return 0; }
    int get_asset_id() override { return 0; }
    int get_client_id() override { return 0; }
    void* get_temp_request_buffer() override { return request_.data(); }
    void* get_temp_response_buffer() override { return response_.data(); }
    bool event_start_service() override
    {
        if (state_ != HAKO_SERVICE_CLIENT_STATE_IDLE) {
            return false;
        }
        state_ = HAKO_SERVICE_CLIENT_STATE_DOING;
        return true;
    }
    bool event_done_service() override
    {
        state_ = HAKO_SERVICE_CLIENT_STATE_IDLE;
        return true;
    }
    bool event_cancel_service() override
    {
        state_ = HAKO_SERVICE_CLIENT_STATE_CANCELING;
        return true;
    }
    int get_request_pdu_size() override { return kPacketSize; }
    int get_response_pdu_size() override { return kPacketSize; }
    std::string get_service_name() override { return kService; }
    std::string get_client_name() override { return kClient; }
    int get_request_channel_id() override { return 0; }
    int get_response_channel_id() override { return 1; }

private:
    alignas(std::max_align_t) std::array<char, kPacketSize> request_ {};
    alignas(std::max_align_t) std::array<char, kPacketSize> response_ {};
    HakoServiceClientStateType state_ = HAKO_SERVICE_CLIENT_STATE_IDLE;
    bool pending_ = false;
};

class HakoServiceInterimReplyTest : public testing::Test {
protected:
    std::shared_ptr<ServerTransport> transport = std::make_shared<ServerTransport>();
    HakoServiceServerProtocol server {transport};

    void SetUp() override
    {
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::temp_directory_path()
            / ("hako-service-interim-reply-" + std::to_string(unique));
        ASSERT_TRUE(std::filesystem::create_directory(root_));
        const auto config = root_ / "cpp_core_config.json";
        std::ofstream output(config);
        output << nlohmann::json {{"shm_type", "mmap"}, {"core_mmap_path", root_.string()}};
        output.close();
        ASSERT_TRUE(output);
#ifdef _WIN32
        ASSERT_EQ(_putenv_s("HAKO_CONFIG_PATH", config.string().c_str()), 0);
#else
        ASSERT_EQ(setenv("HAKO_CONFIG_PATH", config.string().c_str(), 1), 0);
#endif
        ASSERT_TRUE(hako::init());
        core_initialized_ = true;
        ASSERT_NE(hako::create_master(), nullptr);
        clock_ = std::make_unique<hako::data::HakoMasterData>();
        ASSERT_TRUE(clock_->load("mmap"));
        ASSERT_TRUE(server.initialize(kService, "server"));
    }

    void TearDown() override
    {
        clock_.reset();
        if (core_initialized_) {
            hako::destroy();
        }
        if (!root_.empty()) {
            std::error_code error;
            std::filesystem::remove_all(root_, error);
        }
    }

    void set_time(HakoTimeType usec)
    {
        // Advance the Core clock using the same mmap seen by the protocol.
        // No conductor thread or wall-clock delay is involved in these tests.
        clock_->lock();
        clock_->ref_time_nolock().current = usec;
        clock_->unlock();
    }

    void expect_response(
        std::size_t index, std::uint32_t id, HakoServiceStatusType status,
        HakoServiceResultCodeType result)
    {
        ASSERT_LT(index, transport->responses.size());
        const auto& response = transport->responses[index];
        EXPECT_EQ(response.client, 0);
        EXPECT_EQ(response.header.request_id, id);
        EXPECT_EQ(response.header.service_name, kService);
        EXPECT_EQ(response.header.client_name, kClient);
        EXPECT_EQ(response.header.status, status);
        EXPECT_EQ(response.header.result_code, result);
    }

    void send_application_response(HakoServiceStatusType status, HakoServiceResultCodeType result)
    {
        auto* packet = static_cast<char*>(server.get_response_buffer(status, result));
        ASSERT_NE(packet, nullptr);
        EXPECT_TRUE(server.reply_to_current_request(packet, server.get_response_pdu_size()));
    }

    void expect_active(HakoServiceServerStateType state)
    {
        EXPECT_EQ(server.state(), state);
        EXPECT_EQ(server.get_current_client_id(), 0);
        EXPECT_EQ(transport->rotations, 0);
        EXPECT_EQ(transport->completions, 0);
    }

    void expect_completed()
    {
        EXPECT_EQ(server.state(), HAKO_SERVICE_SERVER_STATE_IDLE);
        EXPECT_EQ(server.get_current_client_id(), 1);
        EXPECT_EQ(transport->rotations, 1);
        EXPECT_EQ(transport->completions, 1);
    }

private:
    std::filesystem::path root_;
    std::unique_ptr<hako::data::HakoMasterData> clock_;
    bool core_initialized_ = false;
};

TEST_F(HakoServiceInterimReplyTest, DoingBusyPreservesTheOriginalRequest)
{
    ASSERT_TRUE(transport->queue_request(41));
    ASSERT_EQ(server.poll(), HAKO_SERVICE_SERVER_REQUEST_IN);
    ASSERT_TRUE(transport->queue_request(89));
    EXPECT_EQ(server.poll(), HAKO_SERVICE_SERVER_EVENT_NONE);

    ASSERT_EQ(transport->responses.size(), 1u);
    expect_response(0, 89, HAKO_SERVICE_STATUS_DOING, HAKO_SERVICE_RESULT_CODE_BUSY);
    expect_active(HAKO_SERVICE_SERVER_STATE_DOING);

    send_application_response(HAKO_SERVICE_STATUS_DONE, HAKO_SERVICE_RESULT_CODE_OK);
    ASSERT_EQ(transport->responses.size(), 2u);
    expect_response(1, 41, HAKO_SERVICE_STATUS_DONE, HAKO_SERVICE_RESULT_CODE_OK);
    expect_completed();
}

TEST_F(HakoServiceInterimReplyTest, CancelingBusyPreservesTheCancellation)
{
    ASSERT_TRUE(transport->queue_request(41));
    ASSERT_EQ(server.poll(), HAKO_SERVICE_SERVER_REQUEST_IN);
    ASSERT_TRUE(transport->queue_request(41, HAKO_SERVICE_OPERATION_CODE_CANCEL));
    ASSERT_EQ(server.poll(), HAKO_SERVICE_SERVER_REQUEST_CANCEL);
    ASSERT_TRUE(transport->queue_request(97));
    EXPECT_EQ(server.poll(), HAKO_SERVICE_SERVER_EVENT_NONE);

    ASSERT_EQ(transport->responses.size(), 1u);
    expect_response(0, 97, HAKO_SERVICE_STATUS_CANCELING, HAKO_SERVICE_RESULT_CODE_BUSY);
    expect_active(HAKO_SERVICE_SERVER_STATE_CANCELING);

    send_application_response(HAKO_SERVICE_STATUS_DONE, HAKO_SERVICE_RESULT_CODE_CANCELED);
    ASSERT_EQ(transport->responses.size(), 2u);
    expect_response(1, 41, HAKO_SERVICE_STATUS_DONE, HAKO_SERVICE_RESULT_CODE_CANCELED);
    expect_completed();
}

TEST_F(HakoServiceInterimReplyTest, ProgressUsesItsConfiguredIntervalDuringFrequentPolling)
{
    set_time(100000);
    ASSERT_TRUE(transport->queue_request(41, HAKO_SERVICE_OPERATION_CODE_REQUEST, 5));
    ASSERT_EQ(server.poll(), HAKO_SERVICE_SERVER_REQUEST_IN);
    ASSERT_TRUE(transport->responses.empty());

    for (int elapsed_ms = 1; elapsed_ms <= 15; ++elapsed_ms) {
        SCOPED_TRACE(elapsed_ms);
        set_time(100000 + 1000 * elapsed_ms);
        server.put_progress(4 * elapsed_ms);
        EXPECT_EQ(server.poll(), HAKO_SERVICE_SERVER_EVENT_NONE);
        const auto expected_count = static_cast<std::size_t>(elapsed_ms / 5);
        ASSERT_EQ(transport->responses.size(), expected_count);
        expect_active(HAKO_SERVICE_SERVER_STATE_DOING);
        if (elapsed_ms % 5 == 0) {
            expect_response(
                expected_count - 1, 41, HAKO_SERVICE_STATUS_DOING, HAKO_SERVICE_RESULT_CODE_OK);
            EXPECT_EQ(transport->responses.back().header.processing_percentage, 4 * elapsed_ms);
        }
    }

    send_application_response(HAKO_SERVICE_STATUS_DONE, HAKO_SERVICE_RESULT_CODE_OK);
    ASSERT_EQ(transport->responses.size(), 4u);
    expect_response(3, 41, HAKO_SERVICE_STATUS_DONE, HAKO_SERVICE_RESULT_CODE_OK);
    expect_completed();
}

TEST_F(HakoServiceInterimReplyTest, ApplicationProgressAlsoPreservesTheRequest)
{
    ASSERT_TRUE(transport->queue_request(41));
    ASSERT_EQ(server.poll(), HAKO_SERVICE_SERVER_REQUEST_IN);

    send_application_response(HAKO_SERVICE_STATUS_DOING, HAKO_SERVICE_RESULT_CODE_OK);
    ASSERT_EQ(transport->responses.size(), 1u);
    expect_response(0, 41, HAKO_SERVICE_STATUS_DOING, HAKO_SERVICE_RESULT_CODE_OK);
    expect_active(HAKO_SERVICE_SERVER_STATE_DOING);

    send_application_response(HAKO_SERVICE_STATUS_ERROR, HAKO_SERVICE_RESULT_CODE_INVALID);
    ASSERT_EQ(transport->responses.size(), 2u);
    expect_response(1, 41, HAKO_SERVICE_STATUS_ERROR, HAKO_SERVICE_RESULT_CODE_INVALID);
    expect_completed();
}

TEST_F(HakoServiceInterimReplyTest, ClientDoesNotSendRequestsWhileDoingOrCanceling)
{
    auto client_transport = std::make_shared<ClientTransport>();
    HakoServiceClientProtocol client(client_transport);
    ASSERT_TRUE(client.initialize(kService, kClient, "asset"));
    auto* packet = static_cast<char*>(client.get_request_buffer(HAKO_SERVICE_OPERATION_CODE_REQUEST, -1));
    ASSERT_NE(packet, nullptr);
    ASSERT_TRUE(client.request(packet, client.get_request_pdu_size(), 1000));
    ASSERT_EQ(client.state(), HAKO_SERVICE_CLIENT_STATE_DOING);
    ASSERT_EQ(client_transport->sends, 1);

    EXPECT_FALSE(client.request(packet, client.get_request_pdu_size(), 1000));
    EXPECT_EQ(client_transport->sends, 1);
    EXPECT_EQ(client.state(), HAKO_SERVICE_CLIENT_STATE_DOING);

    ASSERT_TRUE(client.cancel_request());
    ASSERT_EQ(client.state(), HAKO_SERVICE_CLIENT_STATE_CANCELING);
    const int sends = client_transport->sends;
    EXPECT_FALSE(client.request(packet, client.get_request_pdu_size(), 1000));
    EXPECT_EQ(client_transport->sends, sends);
    EXPECT_EQ(client.state(), HAKO_SERVICE_CLIENT_STATE_CANCELING);
}

TEST_F(HakoServiceInterimReplyTest, ClientAcceptsProgressAtExactIntervalsAndIgnoresBusy)
{
    auto client_transport = std::make_shared<ClientTransport>();
    HakoServiceClientProtocol client(client_transport);
    ASSERT_TRUE(client.initialize(kService, kClient, "asset"));
    set_time(100000);

    auto* packet = static_cast<char*>(client.get_request_buffer(HAKO_SERVICE_OPERATION_CODE_REQUEST, 5));
    ASSERT_NE(packet, nullptr);
    HakoCpp_ServiceRequestHeader request {};
    ASSERT_TRUE(RequestConverter {}.pdu2cpp(packet, request));
    ASSERT_TRUE(client.request(packet, client.get_request_pdu_size(), 1000));

    for (int elapsed_ms = 5; elapsed_ms <= 15; elapsed_ms += 5) {
        SCOPED_TRACE(elapsed_ms);
        set_time(100000 + 1000 * elapsed_ms);
        const auto progress = static_cast<std::uint8_t>(elapsed_ms * 5);
        ASSERT_TRUE(client_transport->queue_response(
            request.request_id, HAKO_SERVICE_STATUS_DOING, HAKO_SERVICE_RESULT_CODE_OK, progress));
        EXPECT_EQ(client.poll(), HAKO_SERVICE_CLIENT_EVENT_NONE);
        EXPECT_EQ(client.get_progress(), progress);
        EXPECT_EQ(client.state(), HAKO_SERVICE_CLIENT_STATE_DOING);
    }

    set_time(121000);
    ASSERT_TRUE(client_transport->queue_response(
        request.request_id + 1, HAKO_SERVICE_STATUS_DOING, HAKO_SERVICE_RESULT_CODE_BUSY, 99));
    EXPECT_EQ(client.poll(), HAKO_SERVICE_CLIENT_EVENT_NONE);
    EXPECT_EQ(client.get_progress(), 75);
    EXPECT_EQ(client.state(), HAKO_SERVICE_CLIENT_STATE_DOING);

    set_time(122000);
    ASSERT_TRUE(client_transport->queue_response(
        request.request_id, HAKO_SERVICE_STATUS_DONE, HAKO_SERVICE_RESULT_CODE_OK, 100));
    EXPECT_EQ(client.poll(), HAKO_SERVICE_CLIENT_RESPONSE_IN);
    EXPECT_EQ(client.state(), HAKO_SERVICE_CLIENT_STATE_IDLE);
}

TEST_F(HakoServiceInterimReplyTest, ClientUsesSameCompletionRuleAsServer)
{
    auto client_transport = std::make_shared<ClientTransport>();
    HakoServiceClientProtocol client(client_transport);
    ASSERT_TRUE(client.initialize(kService, kClient, "asset"));
    auto* packet = static_cast<char*>(client.get_request_buffer(HAKO_SERVICE_OPERATION_CODE_REQUEST, -1));
    ASSERT_NE(packet, nullptr);
    HakoCpp_ServiceRequestHeader request {};
    ASSERT_TRUE(RequestConverter {}.pdu2cpp(packet, request));
    ASSERT_TRUE(client.request(packet, client.get_request_pdu_size(), 1000));

    ASSERT_TRUE(client_transport->queue_response(
        request.request_id, HAKO_SERVICE_STATUS_ERROR, HAKO_SERVICE_RESULT_CODE_ERROR, 0));
    EXPECT_EQ(client.poll(), HAKO_SERVICE_CLIENT_EVENT_NONE);
    EXPECT_EQ(client.state(), HAKO_SERVICE_CLIENT_STATE_DOING);

    ASSERT_TRUE(client_transport->queue_response(
        request.request_id, HAKO_SERVICE_STATUS_ERROR, HAKO_SERVICE_RESULT_CODE_INVALID, 0));
    EXPECT_EQ(client.poll(), HAKO_SERVICE_CLIENT_RESPONSE_IN);
    EXPECT_EQ(client.state(), HAKO_SERVICE_CLIENT_STATE_IDLE);
}

} // namespace
