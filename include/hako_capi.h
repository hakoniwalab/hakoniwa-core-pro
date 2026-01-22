#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include "hako_capi_types.h"
#include "hako_asset_service.h"

/*
 * Deprecated API: kept only for backward compatibility and will be removed
 * in a future release. Please migrate to the newer interfaces.
 */

/*
 * for master
 */
	HAKO_API bool hako_master_init();
	HAKO_API bool hako_master_execute();
	HAKO_API void hako_master_set_config_simtime(hako_time_t max_delay_time_usec, hako_time_t delta_time_usec);
	HAKO_API hako_time_t hako_master_get_max_deltay_time_usec();
	HAKO_API hako_time_t hako_master_get_delta_time_usec();

/*
 * for asset
 */
	HAKO_API bool hako_asset_init();
	HAKO_API bool hako_capi_asset_register(const char* name, hako_asset_callback_t *callbacks);

	HAKO_API bool hako_asset_register_polling(const char* name);
	HAKO_API int  hako_asset_get_event(const char* name);

	HAKO_API bool hako_asset_unregister(const char* name);
	HAKO_API void hako_asset_notify_simtime(const char* name, hako_time_t simtime);
	HAKO_API hako_time_t hako_asset_get_worldtime();
	HAKO_API hako_time_t hako_asset_get_min_asset_time();

	HAKO_API bool hako_asset_start_feedback(const char* asset_name, bool isOk);
	HAKO_API bool hako_asset_stop_feedback(const char* asset_name, bool isOk);
	HAKO_API bool hako_asset_reset_feedback(const char* asset_name, bool isOk);


	HAKO_API bool hako_asset_create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size);
	HAKO_API bool hako_asset_create_pdu_lchannel(const char* robo_name, HakoPduChannelIdType channel_id, size_t pdu_size);
	HAKO_API HakoPduChannelIdType hako_asset_get_pdu_channel(const char* robo_name, HakoPduChannelIdType channel_id);
	HAKO_API bool hako_asset_is_pdu_dirty(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id);
	HAKO_API bool hako_asset_write_pdu(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len);
	HAKO_API bool hako_asset_read_pdu(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len);
	HAKO_API bool hako_asset_write_pdu_nolock(const char* robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len);
	HAKO_API bool hako_asset_read_pdu_nolock(const char* robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len);
	HAKO_API void hako_asset_notify_read_pdu_done(const char* asset_name);
	HAKO_API void hako_asset_notify_write_pdu_done(const char* asset_name);
	HAKO_API bool hako_asset_is_pdu_sync_mode(const char* asset_name);
	HAKO_API bool hako_asset_is_simulation_mode();
	HAKO_API bool hako_asset_is_pdu_created();


	HAKO_API bool hako_capi_asset_register_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel);
	HAKO_API bool hako_capi_asset_check_data_recv_event(const char* asset_name, const char *robo_name, HakoPduChannelIdType lchannel);

/*
 * for simevent
 */
	HAKO_API bool hako_simevent_init();
	HAKO_API int hako_simevent_get_state();
	HAKO_API bool hako_simevent_start();
	HAKO_API bool hako_simevent_stop();
	HAKO_API bool hako_simevent_reset();

#ifdef __cplusplus
}
#endif
