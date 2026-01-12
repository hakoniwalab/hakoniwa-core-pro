#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "hako_capi_types.h"
#include "hako_asset_service.h"

/*
 * New C API: return 0 on success, non-zero on failure.
 * For "is_*" queries, return 1 (true) or 0 (false).
 * This header replaces the deprecated hako_capi.h API surface.
 */

/*
 * for master
 */
HAKO_API int hakoniwa_master_init(void);
HAKO_API int hakoniwa_master_execute(void);
HAKO_API void hakoniwa_master_set_config_simtime(hako_time_t max_delay_time_usec, hako_time_t delta_time_usec);
HAKO_API hako_time_t hakoniwa_master_get_max_deltay_time_usec(void);
HAKO_API hako_time_t hakoniwa_master_get_delta_time_usec(void);

/*
 * for asset
 */
HAKO_API int hakoniwa_asset_init(void);
HAKO_API int hakoniwa_asset_register(const char* name, hako_asset_callback_t *callbacks);

HAKO_API int hakoniwa_asset_register_polling(const char* name);
HAKO_API int hakoniwa_asset_get_event(const char* name);

HAKO_API int hakoniwa_asset_unregister(const char* name);
HAKO_API void hakoniwa_asset_notify_simtime(const char* name, hako_time_t simtime);
HAKO_API hako_time_t hakoniwa_asset_get_worldtime(void);

HAKO_API int hakoniwa_asset_start_feedback(const char* asset_name, bool isOk);
HAKO_API int hakoniwa_asset_stop_feedback(const char* asset_name, bool isOk);
HAKO_API int hakoniwa_asset_reset_feedback(const char* asset_name, bool isOk);

HAKO_API int hakoniwa_asset_create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size);
HAKO_API int hakoniwa_asset_create_pdu_lchannel(const char* robo_name, HakoPduChannelIdType channel_id, size_t pdu_size);
HAKO_API HakoPduChannelIdType hakoniwa_asset_get_pdu_channel(const char* robo_name, HakoPduChannelIdType channel_id);
HAKO_API int hakoniwa_asset_is_pdu_dirty(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id);
HAKO_API int hakoniwa_asset_write_pdu(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len);
HAKO_API int hakoniwa_asset_read_pdu(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len);
HAKO_API int hakoniwa_asset_write_pdu_nolock(const char* robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len);
HAKO_API int hakoniwa_asset_read_pdu_nolock(const char* robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len);
HAKO_API void hakoniwa_asset_notify_read_pdu_done(const char* asset_name);
HAKO_API void hakoniwa_asset_notify_write_pdu_done(const char* asset_name);
HAKO_API int hakoniwa_asset_is_pdu_sync_mode(const char* asset_name);
HAKO_API int hakoniwa_asset_is_simulation_mode(void);
HAKO_API int hakoniwa_asset_is_pdu_created(void);

HAKO_API int hakoniwa_asset_register_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel);
HAKO_API int hakoniwa_asset_check_data_recv_event(const char* asset_name, const char *robo_name, HakoPduChannelIdType lchannel);

/*
 * for simevent
 */
HAKO_API int hakoniwa_simevent_init(void);
HAKO_API int hakoniwa_simevent_get_state(void);
HAKO_API int hakoniwa_simevent_start(void);
HAKO_API int hakoniwa_simevent_stop(void);
HAKO_API int hakoniwa_simevent_reset(void);

#ifdef __cplusplus
}
#endif
