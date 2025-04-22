#pragma once

#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the service.
 * @param service_config_path The path to the service configuration file.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_initialize(const char* service_config_path);

/**
 * Create a service server for a specific service name.
 *
 * @param assetName The name of the asset.
 * @param serviceName The name of the service.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_server_create(const char* assetName, const char* serviceName);

#define HAKO_SERVICE_SERVER_API_EVENT_NONE        0
#define HAKO_SERVICE_SERVER_API_REQUEST_IN        1
#define HAKO_SERVICE_SERVER_API_REQUEST_CANCEL    2
/**
 * Poll the service server for incoming requests.
 * @param service_id The ID of the service.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_server_poll(int service_id);

#define HAKO_SERVICE_SERVER_API_STATUS_IDLE        0
#define HAKO_SERVICE_SERVER_API_STATUS_DOING       1
#define HAKO_SERVICE_SERVER_API_STATUS_CANCELING   2
/*
 * Get the status of the service server.
 * @param service_id The ID of the service.
 * @param status The status of the service server.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_server_status(int service_id, int* status);

/**
 * Get the request packet 
 * @param service_id The ID of the service.
 * @param packet The buffer to store the request packet.
 * @param packet_len The length of the buffer.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_server_get_request(int service_id, char** packet, size_t *packet_len);

#define HAKO_SERVICE_API_STATUS_NONE 0
#define HAKO_SERVICE_API_STATUS_DOING 1
#define HAKO_SERVICE_API_STATUS_CANCELING 2
#define HAKO_SERVICE_API_STATUS_DONE 3
#define HAKO_SERVICE_API_STATUS_ERROR 4

#define HAKO_SERVICE_API_RESULT_CODE_OK 0
#define HAKO_SERVICE_API_RESULT_CODE_ERROR 1
#define HAKO_SERVICE_API_RESULT_CODE_TIMEOUT 2
#define HAKO_SERVICE_API_RESULT_CODE_CANCEL 3
#define HAKO_SERVICE_API_RESULT_CODE_INVALID 4
#define HAKO_SERVICE_API_RESULT_CODE_BUSY 5
#define HAKO_SERVICE_API_RESULT_CODE_NOT_FOUND 6
#define HAKO_SERVICE_API_RESULT_CODE_NOT_SUPPORTED 7
#define HAKO_SERVICE_API_RESULT_CODE_NOT_READY 8
#define HAKO_SERVICE_API_RESULT_CODE_NOT_AVAILABLE 9
#define HAKO_SERVICE_API_RESULT_CODE_NOT_IMPLEMENTED 10
/**
 * Get the response packet from the service server.
 * @param service_id The ID of the service.
 * @param packet The buffer to store the response packet.
 * @param packet_len The length of the buffer.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_server_get_response_buffer(int service_id, char** packet, size_t *packet_len, int status, int result_code);

/**
 * Send a response packet to the client.
 * @param service_id The ID of the service.
 * @param packet The response packet.
 * @param packet_len The length of the response packet.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_server_put_response(int service_id, char* packet, size_t packet_len);

/**
 * Check if a service server is running.
 * @param service_id The ID of the service.
 * @return 1 if the service server is running, 0 if not, -1 on canceled.
 */
extern int hako_asset_service_server_is_canceled(int service_id);

/**
 * Set the status of the service server.
 *
 * @param percentage The percentage of completion (0-100).
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_server_set_progress(int service_id, int percentage);

typedef struct {
    int service_id;
    int client_id;
} HakoServiceHandleType;
/**
 * Create a service client for a specific service name.
 * @param serviceName The name of the service.
 * @param clientName The name of the client.
 * @param handle The handle to be used for the service client.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_create(const char* assetName, const char* serviceName, const char* clientName, HakoServiceHandleType* handle);

#define HAKO_SERVICE_CLIENT_API_EVENT_NONE 0
#define HAKO_SERVICE_CLIENT_API_RESPONSE_IN 1
#define HAKO_SERVICE_CLIENT_API_REQUEST_TIMEOUT 2
#define HAKO_SERVICE_CLIENT_API_REQUEST_CANCEL_DONE 3

/**
 * Poll the service client for incoming responses.
 * @param handle The handle of the service client.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_poll(const HakoServiceHandleType* handle);

#define HAKO_SERVICE_CLIENT_API_OPCODE_REQUEST 0
#define HAKO_SERVICE_CLIENT_API_OPCODE_CANCEL 1
/**
 * Get the request packet buffer from the service client.
 * @param handle The handle of the service client.
 * @param packet The buffer to store the request packet.
 * @param packet_len The length of the buffer.
 * @param opcode The operation code (request or cancel).
 * @param poll_interval_msec The polling interval in milliseconds.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_get_request_buffer(const HakoServiceHandleType* handle, char** packet, size_t *packet_len, int opcode, int poll_interval_msec);


/**
 * Call a service request.
 * @param handle The handle of the service client.
 * @param packet The request packet.
 * @param packet_len The length of the request packet.
 * @param timeout_msec The timeout for the request in milliseconds.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_call_request(const HakoServiceHandleType* handle, char *packet, size_t packet_len, int timeout_msec);

/**
 * Get the response packet from the service server.
 * @param handle The handle of the service client.
 * @param packet The buffer to store the response packet.
 * @param packet_len The length of the buffer.
 * @param timeout The timeout for the response in milliseconds.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_get_response(const HakoServiceHandleType* handle, char **packet, size_t *packet_len, int timeout);

/**
 * Cancel the service request.
 * @param handle The handle of the service client.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_cancel_request(const HakoServiceHandleType* handle);

/**
 * Get the progress of the service client.
 * @param handle The handle of the service client.
 * @return The progress of the service client (0-100).
 */
extern int hako_asset_service_client_get_progress(const HakoServiceHandleType* handle);

#define HAKO_SERVICE_CLIENT_API_STATE_IDLE 0
#define HAKO_SERVICE_CLIENT_API_STATE_DOING 1
#define HAKO_SERVICE_CLIENT_API_STATE_CANCELING 2

/**
 * Check the status of the service client.
 * @param handle The handle of the service client.
 * @param status The status of the service client.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_status(const HakoServiceHandleType* handle, int* status);

#ifdef __cplusplus
}
#endif
