#pragma once

#include "hako_asset_service.h"

/*
 * Internal C++-linkage declarations.  These functions may throw; the public
 * extern "C" entry points in hako_asset_service_guard.cpp contain them and
 * translate failures into the thread-local runtime error contract.
 */
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
