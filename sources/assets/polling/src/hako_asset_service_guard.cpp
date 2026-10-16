#include "hako_asset_service.h"
#include "hako_runtime_error_internal.hpp"

extern "C" {
int hako_asset_service_initialize_unguarded(const char*);
int hako_asset_service_server_create_unguarded(const char*, const char*);
int hako_asset_service_server_poll_unguarded(int);
int hako_asset_service_server_get_current_client_id_unguarded(int);
int hako_asset_service_server_get_current_channel_id_unguarded(int, int*, int*);
int hako_asset_service_server_status_unguarded(int, int*);
int hako_asset_service_server_get_request_unguarded(int, char**, size_t*);
int hako_asset_service_server_get_response_buffer_unguarded(int, char**, size_t*, int, int);
int hako_asset_service_server_put_response_unguarded(int, char*, size_t);
int hako_asset_service_server_is_canceled_unguarded(int);
int hako_asset_service_server_set_progress_unguarded(int, int);
int hako_asset_service_client_create_unguarded(const char*, const char*, const char*, HakoServiceHandleType*);
int hako_asset_service_client_poll_unguarded(const HakoServiceHandleType*);
int hako_asset_service_client_get_channel_id_unguarded(int, int*, int*);
int hako_asset_service_client_get_channel_id_by_handle_unguarded(const HakoServiceHandleType*, int*, int*);
int hako_asset_service_client_get_request_buffer_unguarded(const HakoServiceHandleType*, char**, size_t*, int, int);
int hako_asset_service_client_call_request_unguarded(const HakoServiceHandleType*, char*, size_t, int);
int hako_asset_service_client_get_response_unguarded(const HakoServiceHandleType*, char**, size_t*, int);
int hako_asset_service_client_cancel_request_unguarded(const HakoServiceHandleType*);
int hako_asset_service_client_get_progress_unguarded(const HakoServiceHandleType*);
int hako_asset_service_client_status_unguarded(const HakoServiceHandleType*, int*);
}

namespace {

template <typename Function>
int guard_service(Function&& function) noexcept
{
    return hako::runtime::error::guard(
        std::forward<Function>(function), -1);
}

} // namespace

#define HAKO_GUARD_SERVICE_1(name, type1, arg1) \
    int name(type1 arg1) { return guard_service([&]() { return name##_unguarded(arg1); }); }

int hako_asset_service_initialize(const char* path) { return guard_service([&]() { return hako_asset_service_initialize_unguarded(path); }); }
int hako_asset_service_server_create(const char* asset, const char* service) { return guard_service([&]() { return hako_asset_service_server_create_unguarded(asset, service); }); }
HAKO_GUARD_SERVICE_1(hako_asset_service_server_poll, int, service_id)
HAKO_GUARD_SERVICE_1(hako_asset_service_server_get_current_client_id, int, service_id)
int hako_asset_service_server_get_current_channel_id(int id, int* request, int* response) { return guard_service([&]() { return hako_asset_service_server_get_current_channel_id_unguarded(id, request, response); }); }
int hako_asset_service_server_status(int id, int* status) { return guard_service([&]() { return hako_asset_service_server_status_unguarded(id, status); }); }
int hako_asset_service_server_get_request(int id, char** packet, size_t* length) { return guard_service([&]() { return hako_asset_service_server_get_request_unguarded(id, packet, length); }); }
int hako_asset_service_server_get_response_buffer(int id, char** packet, size_t* length, int status, int result) { return guard_service([&]() { return hako_asset_service_server_get_response_buffer_unguarded(id, packet, length, status, result); }); }
int hako_asset_service_server_put_response(int id, char* packet, size_t length) { return guard_service([&]() { return hako_asset_service_server_put_response_unguarded(id, packet, length); }); }
HAKO_GUARD_SERVICE_1(hako_asset_service_server_is_canceled, int, service_id)
int hako_asset_service_server_set_progress(int id, int percentage) { return guard_service([&]() { return hako_asset_service_server_set_progress_unguarded(id, percentage); }); }
int hako_asset_service_client_create(const char* asset, const char* service, const char* client, HakoServiceHandleType* handle) { return guard_service([&]() { return hako_asset_service_client_create_unguarded(asset, service, client, handle); }); }
HAKO_GUARD_SERVICE_1(hako_asset_service_client_poll, const HakoServiceHandleType*, handle)
int hako_asset_service_client_get_channel_id(int id, int* request, int* response) { return guard_service([&]() { return hako_asset_service_client_get_channel_id_unguarded(id, request, response); }); }
int hako_asset_service_client_get_channel_id_by_handle(const HakoServiceHandleType* handle, int* request, int* response) { return guard_service([&]() { return hako_asset_service_client_get_channel_id_by_handle_unguarded(handle, request, response); }); }
int hako_asset_service_client_get_request_buffer(const HakoServiceHandleType* handle, char** packet, size_t* length, int opcode, int interval) { return guard_service([&]() { return hako_asset_service_client_get_request_buffer_unguarded(handle, packet, length, opcode, interval); }); }
int hako_asset_service_client_call_request(const HakoServiceHandleType* handle, char* packet, size_t length, int timeout) { return guard_service([&]() { return hako_asset_service_client_call_request_unguarded(handle, packet, length, timeout); }); }
int hako_asset_service_client_get_response(const HakoServiceHandleType* handle, char** packet, size_t* length, int timeout) { return guard_service([&]() { return hako_asset_service_client_get_response_unguarded(handle, packet, length, timeout); }); }
HAKO_GUARD_SERVICE_1(hako_asset_service_client_cancel_request, const HakoServiceHandleType*, handle)
HAKO_GUARD_SERVICE_1(hako_asset_service_client_get_progress, const HakoServiceHandleType*, handle)
int hako_asset_service_client_status(const HakoServiceHandleType* handle, int* status) { return guard_service([&]() { return hako_asset_service_client_status_unguarded(handle, status); }); }

#undef HAKO_GUARD_SERVICE_1

