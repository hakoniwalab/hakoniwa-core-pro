#pragma once

#include <stddef.h>
#include <stdint.h>
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

#define HAKO_SERVICE_SERVER_EVENT_EVENT_NONE        0
#define HAKO_SERVICE_SERVER_EVENT_REQUEST_IN        1
#define HAKO_SERVICE_SERVER_EVENT_REQUEST_CANCEL    2
/**
 * Poll the service server for incoming requests.
 * @param service_id The ID of the service.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_server_poll(int service_id);

/**
 * Get the request packet 
 * @param service_id The ID of the service.
 * @param packet The buffer to store the request packet.
 * @param packet_len The length of the buffer.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_server_get_request(int service_id, char** packet, size_t *packet_len);

/**
 * Get the response packet from the service server.
 * @param service_id The ID of the service.
 * @param packet The buffer to store the response packet.
 * @param packet_len The length of the buffer.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_get_response_buffer(int service_id, char** packet, size_t *packet_len);

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
extern int hako_asset_service_client_create(const char* serviceName, const char* clientName, HakoServiceHandleType* handle);

/**
 * Call a service request.
 * @param handle The handle of the service client.
 * @param packet The request packet.
 * @param packet_len The length of the request packet.
 * @param timeout The timeout for the request in milliseconds.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_call_request(const HakoServiceHandleType* handle, char *packet, size_t packet_len, int timeout);

/**
 * Get the response packet from the service server.
 * @param handle The handle of the service client.
 * @param packet The buffer to store the response packet.
 * @param packet_len The length of the buffer.
 * @param timeout The timeout for the response in milliseconds.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_get_response(const HakoServiceHandleType* handle, char *packet, size_t packet_len, int timeout);

/**
 * Cancel the service request.
 * @param handle The handle of the service client.
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_cancel_request(const HakoServiceHandleType* handle);
/**
 * Check the status of the service client.
 * @param handle The handle of the service client.
 * @param status The status of the service client (0: not started, 1: in progress, 2: completed).
 * @param percentage The percentage of completion (0-100).
 * @return 0 on success, -1 on failure.
 */
extern int hako_asset_service_client_status(const HakoServiceHandleType* handle, int* status, int* percentage);
