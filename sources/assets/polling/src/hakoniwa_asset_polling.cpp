#include "hakoniwa_asset_polling.h"
#include "hako_capi.h"

static inline int hako_status_from_bool(bool ok)
{
    return ok ? 0 : -1;
}

/*
 * for master
 */
int hakoniwa_master_init(void)
{
    return hako_status_from_bool(hako_master_init());
}

int hakoniwa_master_execute(void)
{
    return hako_status_from_bool(hako_master_execute());
}

void hakoniwa_master_set_config_simtime(hako_time_t max_delay_time_usec, hako_time_t delta_time_usec)
{
    hako_master_set_config_simtime(max_delay_time_usec, delta_time_usec);
}

hako_time_t hakoniwa_master_get_max_deltay_time_usec(void)
{
    return hako_master_get_max_deltay_time_usec();
}

hako_time_t hakoniwa_master_get_delta_time_usec(void)
{
    return hako_master_get_delta_time_usec();
}

/*
 * for asset
 */
int hakoniwa_asset_init(void)
{
    return hako_status_from_bool(hako_asset_init());
}

int hakoniwa_asset_register(const char* name, hako_asset_callback_t *callbacks)
{
    return hako_status_from_bool(hako_capi_asset_register(name, callbacks));
}

int hakoniwa_asset_register_polling(const char* name)
{
    return hako_status_from_bool(hako_asset_register_polling(name));
}

int hakoniwa_asset_get_event(const char* name)
{
    return hako_asset_get_event(name);
}

int hakoniwa_asset_unregister(const char* name)
{
    return hako_status_from_bool(hako_asset_unregister(name));
}

void hakoniwa_asset_notify_simtime(const char* name, hako_time_t simtime)
{
    hako_asset_notify_simtime(name, simtime);
}

hako_time_t hakoniwa_asset_get_worldtime(void)
{
    return hako_asset_get_worldtime();
}
hako_time_t hakoniwa_asset_get_min_asset_time(void)
{
    return hako_asset_get_min_asset_time();
}

int hakoniwa_asset_start_feedback(const char* asset_name, bool isOk)
{
    return hako_status_from_bool(hako_asset_start_feedback(asset_name, isOk));
}

int hakoniwa_asset_stop_feedback(const char* asset_name, bool isOk)
{
    return hako_status_from_bool(hako_asset_stop_feedback(asset_name, isOk));
}

int hakoniwa_asset_reset_feedback(const char* asset_name, bool isOk)
{
    return hako_status_from_bool(hako_asset_reset_feedback(asset_name, isOk));
}

int hakoniwa_asset_start_feedback_ok(const char* asset_name)
{
    return hako_status_from_bool(hako_asset_start_feedback(asset_name, true));
}

int hakoniwa_asset_stop_feedback_ok(const char* asset_name)
{
    return hako_status_from_bool(hako_asset_stop_feedback(asset_name, true));
}

int hakoniwa_asset_reset_feedback_ok(const char* asset_name)
{
    return hako_status_from_bool(hako_asset_reset_feedback(asset_name, true));
}

int hakoniwa_asset_create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size)
{
    return hako_status_from_bool(hako_asset_create_pdu_channel(channel_id, pdu_size));
}

int hakoniwa_asset_create_pdu_lchannel(const char* robo_name, HakoPduChannelIdType channel_id, size_t pdu_size)
{
    return hako_status_from_bool(hako_asset_create_pdu_lchannel(robo_name, channel_id, pdu_size));
}

HakoPduChannelIdType hakoniwa_asset_get_pdu_channel(const char* robo_name, HakoPduChannelIdType channel_id)
{
    return hako_asset_get_pdu_channel(robo_name, channel_id);
}

int hakoniwa_asset_is_pdu_dirty(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id)
{
    return hako_asset_is_pdu_dirty(asset_name, robo_name, channel_id) ? 1 : 0;
}

int hakoniwa_asset_write_pdu(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)
{
    return hako_status_from_bool(hako_asset_write_pdu(asset_name, robo_name, channel_id, pdu_data, len));
}

int hakoniwa_asset_read_pdu(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len)
{
    return hako_status_from_bool(hako_asset_read_pdu(asset_name, robo_name, channel_id, pdu_data, len));
}

int hakoniwa_asset_write_pdu_nolock(const char* robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)
{
    return hako_status_from_bool(hako_asset_write_pdu_nolock(robo_name, channel_id, pdu_data, len));
}

int hakoniwa_asset_read_pdu_nolock(const char* robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len)
{
    return hako_status_from_bool(hako_asset_read_pdu_nolock(robo_name, channel_id, pdu_data, len));
}

void hakoniwa_asset_notify_read_pdu_done(const char* asset_name)
{
    hako_asset_notify_read_pdu_done(asset_name);
}

void hakoniwa_asset_notify_write_pdu_done(const char* asset_name)
{
    hako_asset_notify_write_pdu_done(asset_name);
}

int hakoniwa_asset_is_pdu_sync_mode(const char* asset_name)
{
    return hako_asset_is_pdu_sync_mode(asset_name) ? 1 : 0;
}

int hakoniwa_asset_is_simulation_mode(void)
{
    return hako_asset_is_simulation_mode() ? 1 : 0;
}

int hakoniwa_asset_is_pdu_created(void)
{
    return hako_asset_is_pdu_created() ? 1 : 0;
}
void hakoniwa_asset_load_pdu_data(void)
{
    (void)hakoniwa_asset_is_pdu_created();
}

int hakoniwa_asset_register_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel)
{
    return hako_status_from_bool(hako_capi_asset_register_data_recv_event(robo_name, lchannel));
}

int hakoniwa_asset_check_data_recv_event(const char* asset_name, const char *robo_name, HakoPduChannelIdType lchannel)
{
    return hako_status_from_bool(hako_capi_asset_check_data_recv_event(asset_name, robo_name, lchannel));
}

/*
 * for simevent
 */
int hakoniwa_simevent_init(void)
{
    return hako_status_from_bool(hako_simevent_init());
}

int hakoniwa_simevent_get_state(void)
{
    return hako_simevent_get_state();
}

int hakoniwa_simevent_start(void)
{
    return hako_status_from_bool(hako_simevent_start());
}

int hakoniwa_simevent_stop(void)
{
    return hako_status_from_bool(hako_simevent_stop());
}

int hakoniwa_simevent_reset(void)
{
    return hako_status_from_bool(hako_simevent_reset());
}
