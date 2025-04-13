#pragma once

#include "hako_primitive_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hako_asset_context_s hako_asset_context_t;
typedef struct hako_asset_callbacks_s hako_asset_callbacks_t;

struct hako_asset_callbacks_s {
    int (*on_initialize)(hako_asset_context_t*);
    int (*on_simulation_step)(hako_asset_context_t*);
    int (*on_manual_timing_control)(hako_asset_context_t*);
    int (*on_reset)(hako_asset_context_t*);
};
#define HAKO_ASSET_MIN_DELTA_TIME_USEC  1
typedef enum {
    HAKO_ASSET_MODEL_PLANT = 0,
    HAKO_ASSET_MODEL_CONTROLLER
} HakoAssetModelType;
extern int hako_asset_register(const char *asset_name, const char *config_path, hako_asset_callbacks_t *callbacks, hako_time_t delta_usec, HakoAssetModelType model);
extern int hako_asset_start(void);
extern int hako_asset_pdu_read(const char *robo_name, HakoPduChannelIdType lchannel, char *buffer, size_t buffer_len);
extern int hako_asset_pdu_write(const char *robo_name, HakoPduChannelIdType lchannel, const char *buffer, size_t buffer_len);
extern hako_time_t hako_asset_simulation_time(void);
extern int hako_asset_usleep(hako_time_t sleep_time_usec);

extern int hako_initialize_for_external(void);
extern int hako_asset_pdu_create(const char *robo_name, HakoPduChannelIdType lchannel, size_t pdu_size);

/**
 * Register a data receive event for a specific logical channel.
 *
 * @param robo_name The name of the robot (virtual asset).
 * @param lchannel The logical PDU channel ID.
 * @param on_recv Callback function to be invoked when data is received.
 *                If NULL, a flag-based mechanism will be used instead.
 * @return true if the registration succeeded, false otherwise.
 */
extern int hako_asset_register_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel, void (*on_recv)(int), int* recv_event_id);

/**
 * Check if a receive event has occurred (only valid when flag-based).
 *
 * @param robo_name The name of the robot (virtual asset).
 * @param lchannel The logical PDU channel ID.
 * @return true if a receive event was detected, false otherwise.
 */
extern int hako_asset_check_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel);

/**
 * Create a service server for a specific service name.
 *
 * @param serviceName The name of the service.
 * @param service Callback function to handle the service requests.
 * @return 0 on success, -1 on failure.
 */
extern int hako_service_server_create(const char* serviceName, int (*service)(int));

/**
 * Get the request packet 
 * @param service_id The ID of the service.
 * @param packet The buffer to store the request packet.
 * @param packet_len The length of the buffer.
 * @return 0 on success, -1 on failure.
 */
extern int hako_service_server_get_request(int service_id, char* packet, size_t packet_len);
/**
 * Send a response packet to the client.
 * @param service_id The ID of the service.
 * @param packet The response packet.
 * @param packet_len The length of the response packet.
 * @return 0 on success, -1 on failure.
 */
extern int hako_service_server_put_response(int service_id, char* packet, size_t packet_len);

/**
 * Check if a service server is running.
 * @param service_id The ID of the service.
 * @return 1 if the service server is running, 0 if not, -1 on canceled.
 */
extern int hako_service_server_is_canceled(int service_id);

/**
 * Set the status of the service server.
 *
 * @param percentage The percentage of completion (0-100).
 * @return 0 on success, -1 on failure.
 */
extern int hako_service_server_set_status(int percentage);

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
extern int hako_service_client_create(const char* serviceName, const char* clientName, HakoServiceHandleType* handle);

/**
 * Call a service request.
 * @param handle The handle of the service client.
 * @param packet The request packet.
 * @param packet_len The length of the request packet.
 * @param timeout The timeout for the request in milliseconds.
 * @return 0 on success, -1 on failure.
 */
extern int hako_service_client_call_request(HakoServiceHandleType* handle, char *packet, size_t packet_len, int timeout);

/**
 * Get the response packet from the service server.
 * @param handle The handle of the service client.
 * @param packet The buffer to store the response packet.
 * @param packet_len The length of the buffer.
 * @param timeout The timeout for the response in milliseconds.
 * @return 0 on success, -1 on failure.
 */
extern int hako_service_client_get_response(HakoServiceHandleType* handle, char *packet, size_t packet_len, int timeout);

/**
 * Cancel the service request.
 * @param handle The handle of the service client.
 * @return 0 on success, -1 on failure.
 */
extern int hako_service_client_cancel_request(HakoServiceHandleType* handle);
/**
 * Check the status of the service client.
 * @param handle The handle of the service client.
 * @param status The status of the service client (0: not started, 1: in progress, 2: completed).
 * @param percentage The percentage of completion (0-100).
 * @return 0 on success, -1 on failure.
 */
extern int hako_service_client_status(HakoServiceHandleType* handle, int* status, int* percentage);

#ifdef __cplusplus
}
#endif

