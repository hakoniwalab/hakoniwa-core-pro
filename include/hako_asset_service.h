#pragma once

#include <stddef.h>
#include <stdint.h>
#include "hako_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Hakoniwa Service API - C Interface
 * ----------------------------------
 * This header defines the API for creating service servers and clients
 * that communicate via shared memory in the Hakoniwa simulation framework.
 * 
 * These functions and constants allow RPC-like service interaction between
 * simulation components.
 */

/* ========== [ Server API ] ========== */

/**
 * Initializes the service layer.
 * 
 * @param service_config_path Path to the service configuration JSON file.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_initialize(const char* service_config_path);

/**
 * Creates a service server with the given asset and service names.
 *
 * @param assetName Asset name registered in the simulation.
 * @param serviceName Unique service identifier.
 * @return Service ID (>=0) on success, -1 on failure.
 */
HAKO_API int hako_asset_service_server_create(const char* assetName, const char* serviceName);

/* Server poll event types */
#define HAKO_SERVICE_SERVER_API_EVENT_NONE         0  // No event
#define HAKO_SERVICE_SERVER_API_REQUEST_IN         1  // New request received
#define HAKO_SERVICE_SERVER_API_REQUEST_CANCEL     2  // Cancel request received

/**
 * Polls the server for incoming requests.
 *
 * @param service_id Service ID returned by creation.
 * @return Event code (see defines above), -1 on error.
 */
HAKO_API int hako_asset_service_server_poll(int service_id);

/**
 * Cancels the current client id
 *
 * @param service_id Service ID.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_server_get_current_client_id(int service_id);
/**
 * Retrieves the current request and response channel IDs.
 *
 * @param service_id Service ID.
 * @param request_channel_id Output pointer to store request channel ID.
 * @param response_channel_id Output pointer to store response channel ID.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_server_get_current_channel_id(int service_id, int* request_channel_id, int* response_channel_id);


/* Server status flags */
#define HAKO_SERVICE_SERVER_API_STATUS_IDLE        0
#define HAKO_SERVICE_SERVER_API_STATUS_DOING       1
#define HAKO_SERVICE_SERVER_API_STATUS_CANCELING   2

/**
 * Gets the current status of the service server.
 *
 * @param service_id Service ID.
 * @param status Output pointer to store status.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_server_status(int service_id, int* status);

/**
 * Retrieves the incoming request packet.
 *
 * @param service_id Service ID.
 * @param packet Output pointer to buffer pointer.
 * @param packet_len Output pointer to buffer size.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_server_get_request(int service_id, char** packet, size_t* packet_len);

/* Common API status/result codes (used in responses: same as HakoServiceStatusType) */
#define HAKO_SERVICE_API_STATUS_NONE         0
#define HAKO_SERVICE_API_STATUS_DOING        1
#define HAKO_SERVICE_API_STATUS_CANCELING    2
#define HAKO_SERVICE_API_STATUS_DONE         3
#define HAKO_SERVICE_API_STATUS_ERROR        4
/* same as HakoServiceResultCodeType */
#define HAKO_SERVICE_API_RESULT_CODE_OK             0
#define HAKO_SERVICE_API_RESULT_CODE_ERROR          1
#define HAKO_SERVICE_API_RESULT_CODE_CANCELED       2
#define HAKO_SERVICE_API_RESULT_CODE_INVALID        3
#define HAKO_SERVICE_API_RESULT_CODE_BUSY           4

/**
 * Retrieves a response buffer to fill.
 *
 * @param service_id Service ID.
 * @param packet Output pointer to buffer.
 * @param packet_len Output pointer to buffer size.
 * @param status Service status.
 * @param result_code Operation result.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_server_get_response_buffer(int service_id, char** packet, size_t* packet_len, int status, int result_code);

/**
 * Sends a response to the client.
 *
 * @param service_id Service ID.
 * @param packet Response data.
 * @param packet_len Length of the data.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_server_put_response(int service_id, char* packet, size_t packet_len);

/**
 * Checks if the server has been canceled.
 *
 * @param service_id Service ID.
 * @return 1 if canceled, 0 if not, -1 on error.
 */
HAKO_API int hako_asset_service_server_is_canceled(int service_id);

/**
 * Sets the service progress.
 *
 * @param service_id Service ID.
 * @param percentage Completion percentage (0-100).
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_server_set_progress(int service_id, int percentage);


/* ========== [ Client API ] ========== */

typedef struct {
    int service_id;
    int client_id;
} HakoServiceHandleType;

/**
 * Creates a service client.
 *
 * @param assetName Asset name.
 * @param serviceName Target service name.
 * @param clientName Client name identifier.
 * @param handle Output handle to the client instance.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_client_create(const char* assetName, const char* serviceName, const char* clientName, HakoServiceHandleType* handle);

/* Client poll event types */
#define HAKO_SERVICE_CLIENT_API_EVENT_NONE             0  // No response
#define HAKO_SERVICE_CLIENT_API_RESPONSE_IN            1  // Response received
#define HAKO_SERVICE_CLIENT_API_REQUEST_TIMEOUT        2  // Timeout occurred
#define HAKO_SERVICE_CLIENT_API_REQUEST_CANCEL_DONE    3  // Cancel completed

/**
 * Polls the client for service responses.
 *
 * @param handle Client handle.
 * @return Event code, -1 on failure.
 */
HAKO_API int hako_asset_service_client_poll(const HakoServiceHandleType* handle);

/**
 * Retrieves the current request and response channel IDs.
 *
 * @param service_id Service ID.
 * @param request_channel_id Output pointer to store request channel ID.
 * @param response_channel_id Output pointer to store response channel ID.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_client_get_current_channel_id(int service_id, int* request_channel_id, int* response_channel_id);

/* Client opcode values (used in request header) */
#define HAKO_SERVICE_CLIENT_API_OPCODE_REQUEST     0
#define HAKO_SERVICE_CLIENT_API_OPCODE_CANCEL      1

/**
 * Retrieves a buffer for the client to send a request.
 *
 * @param handle Client handle.
 * @param packet Output buffer pointer.
 * @param packet_len Output buffer size.
 * @param opcode Request type (see defines above).
 * @param poll_interval_msec Polling interval for progress reports.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_client_get_request_buffer(const HakoServiceHandleType* handle, char** packet, size_t* packet_len, int opcode, int poll_interval_msec);

/**
 * Sends a request to the server.
 *
 * @param handle Client handle.
 * @param packet Request packet.
 * @param packet_len Length of the request.
 * @param timeout_msec Timeout for the operation.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_client_call_request(const HakoServiceHandleType* handle, char* packet, size_t packet_len, int timeout_msec);

/**
 * Retrieves the response packet from the server.
 *
 * @param handle Client handle.
 * @param packet Output pointer to buffer.
 * @param packet_len Output pointer to buffer size.
 * @param timeout Timeout in milliseconds.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_client_get_response(const HakoServiceHandleType* handle, char** packet, size_t* packet_len, int timeout);

/**
 * Requests cancellation of the current operation.
 *
 * @param handle Client handle.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_client_cancel_request(const HakoServiceHandleType* handle);

/**
 * Retrieves current progress of the operation.
 *
 * @param handle Client handle.
 * @return Progress value (0-100), or -1 on error.
 */
HAKO_API int hako_asset_service_client_get_progress(const HakoServiceHandleType* handle);

/* Client internal state flags */
#define HAKO_SERVICE_CLIENT_API_STATE_IDLE         0
#define HAKO_SERVICE_CLIENT_API_STATE_DOING        1
#define HAKO_SERVICE_CLIENT_API_STATE_CANCELING    2

/**
 * Gets the current status of the service client.
 *
 * @param handle Client handle.
 * @param status Output pointer to store status.
 * @return 0 on success, -1 on failure.
 */
HAKO_API int hako_asset_service_client_status(const HakoServiceHandleType* handle, int* status);

#ifdef __cplusplus
}
#endif
