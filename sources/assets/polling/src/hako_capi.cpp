#include "hako_pro.hpp"
#include "hako_capi.h"
#include <fstream>
#include <iostream>
#ifdef ENABLE_HAKO_TIME_MEASURE
#include "hako_asset_impl_measure.hpp"
static void* hako_measure_vp;
#endif /* ENABLE_HAKO_TIME_MEASURE */

static std::shared_ptr<hako::IHakoMasterController>  hako_master;
static std::shared_ptr<hako::IHakoSimulationEventController>  hako_simevent;
static std::shared_ptr<hako::IHakoAssetController>  hako_asset;

/*
 * for master
 */
bool hako_master_init()
{
    try {
        hako::init();
        hako_master = hako::create_master();
        if (hako_master == nullptr) {
            return false;
        }
        hako_simevent = hako::get_simevent_controller();
        if (hako_simevent == nullptr) {
            return false;
        }
        return true;
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_master_execute()
{
    try {
        return hako_master->execute();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

void hako_master_set_config_simtime(hako_time_t max_delay_time_usec, hako_time_t delta_time_usec)
{
    try {
        //hako::logger::get("master")->info("max_delay={0} usec delta={1} usec", max_delay_time_usec, delta_time_usec);
        hako_master->set_config_simtime(max_delay_time_usec, delta_time_usec);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return;
    }
    return;
}
hako_time_t hako_master_get_max_deltay_time_usec()
{
    try {
        return hako_master->get_max_deltay_time_usec();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return 0;
    }
}
hako_time_t hako_master_get_delta_time_usec()
{
    try {
        return hako_master->get_delta_time_usec();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return 0;
    }
}

/*
 * for asset
 */
bool hako_asset_init()
{
    static bool log_is_initalized = false;
    try {
        if (log_is_initalized == false) {
            log_is_initalized = true;
        }
        hako_asset = hako::create_asset_controller();
        if (hako_asset == nullptr) {
            std::cout << "ERROR: Not found hako-master on this PC" << std::endl;
            return false;
        }
        hako_simevent = hako::get_simevent_controller();
        if (hako_simevent == nullptr) {
            return false;
        }
        return true;
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_register(const char* name, hako_asset_callback_t *callbacks)
{
    try {
#ifdef ENABLE_HAKO_TIME_MEASURE
        hako_measure_vp = hako_asset_impl_measure_create_csv((const char*)name);
        HAKO_IMPL_ASSERT(hako_measure_vp != nullptr);
#endif /* ENABLE_HAKO_TIME_MEASURE */
        std::string asset_name(name);
        if (callbacks != nullptr) {
            AssetCallbackType cbk;
            cbk.start = callbacks->start;
            cbk.stop = callbacks->stop;
            cbk.reset = callbacks->reset;
            return hako_asset->asset_register(asset_name, cbk);
        }
        else {
            return hako_asset->asset_register_polling(asset_name);
        }
    }
    catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}
bool hako_asset_register_polling(const char* name)
{
    return hako_asset_register(name, nullptr);
}
int  hako_asset_get_event(const char* name)
{
    try {
        return (int)hako_asset->asset_get_event(name);
    }
    catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return (int)HakoSimulationAssetEventType::HakoSimAssetEvent_Error;
    }
}

bool hako_asset_unregister(const char* name)
{
    try {
        std::string asset_name(name);
        return hako_asset->asset_unregister(asset_name);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

void hako_asset_notify_simtime(const char* name, hako_time_t simtime)
{
    try {
#ifdef ENABLE_HAKO_TIME_MEASURE
        hako_asset_impl_measure_write_csv(hako_measure_vp, 
            (long long int)hako_asset_get_worldtime(), 
            (long long int)simtime);
#endif /* ENABLE_HAKO_TIME_MEASURE */
        std::string asset_name(name);
        hako_asset->notify_simtime(asset_name, (HakoTimeType)simtime);
    }
    catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return;
    }
}

hako_time_t hako_asset_get_worldtime()
{
    try {
        return (hako_time_t)hako_asset->get_worldtime();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return 0;
    }
}


bool hako_asset_start_feedback(const char* asset_name, bool isOk)
{
    try {
        std::string hako_asset_name(asset_name);
        return hako_asset->start_feedback(hako_asset_name, isOk);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_stop_feedback(const char* asset_name, bool isOk)
{
    try {
#ifdef ENABLE_HAKO_TIME_MEASURE
        hako_asset_impl_measure_flush_csv(hako_measure_vp);
#endif /* ENABLE_HAKO_TIME_MEASURE */
        std::string hako_asset_name(asset_name);
        return hako_asset->stop_feedback(hako_asset_name, isOk);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_reset_feedback(const char* asset_name, bool isOk)
{
    try {
        std::string hako_asset_name(asset_name);
        return hako_asset->reset_feedback(hako_asset_name, isOk);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size)
{
    try {
        hako_asset_init();//TODO
        return hako_asset->create_pdu_channel(channel_id, pdu_size);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}
bool hako_asset_create_pdu_lchannel(const char* robo_name, HakoPduChannelIdType channel_id, size_t pdu_size)
{
    try {
        std::string hako_robo_name(robo_name);
        hako_asset_init();//TODO
        return hako_asset->create_pdu_lchannel(hako_robo_name, channel_id, pdu_size);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}
HakoPduChannelIdType hako_asset_get_pdu_channel(const char* robo_name, HakoPduChannelIdType channel_id)
{
    try {
        std::string hako_robo_name(robo_name);
        hako_asset_init();//TODO
        return hako_asset->get_pdu_channel(hako_robo_name, channel_id);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return -1;
    }
}

bool hako_asset_is_pdu_dirty(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id)
{
    try {
        std::string hako_asset_name(asset_name);
        std::string hako_robo_name(robo_name);
        return hako_asset->is_pdu_dirty(hako_asset_name, hako_robo_name, channel_id);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_write_pdu(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)
{
    try {
        std::string hako_asset_name(asset_name);
        std::string hako_robo_name(robo_name);
        return hako_asset->write_pdu(hako_asset_name, hako_robo_name, channel_id, pdu_data, len);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_read_pdu(const char* asset_name, const char* robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len)
{
    try {
        std::string hako_asset_name(asset_name);
        std::string hako_robo_name(robo_name);
        return hako_asset->read_pdu(hako_asset_name, hako_robo_name, channel_id, pdu_data, len);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_write_pdu_nolock(const char* robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)
{
    try {
        std::string hako_robo_name(robo_name);
        return hako_asset->write_pdu_nolock(hako_robo_name, channel_id, pdu_data, len);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_read_pdu_nolock(const char* robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len)
{
    try {
        std::string hako_robo_name(robo_name);
        return hako_asset->read_pdu_nolock(hako_robo_name, channel_id, pdu_data, len);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

void hako_asset_notify_read_pdu_done(const char* asset_name)
{
    try {
        std::string hako_asset_name(asset_name);
        hako_asset->notify_read_pdu_done(hako_asset_name);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return;
    }
}

void hako_asset_notify_write_pdu_done(const char* asset_name)
{
    try {
        std::string hako_asset_name(asset_name);
        hako_asset->notify_write_pdu_done(hako_asset_name);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return;
    }
}

bool hako_asset_is_pdu_sync_mode(const char* asset_name)
{
    try {
        std::string hako_asset_name(asset_name);
        return hako_asset->is_pdu_sync_mode(hako_asset_name);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_is_simulation_mode()
{
    try {
        return hako_asset->is_simulation_mode();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_asset_is_pdu_created()
{
    try {
        return hako_asset->is_pdu_created();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}
bool hako_asset_register_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel)
{
    try {
        auto pro_data = hako::data::pro::hako_pro_get_data();
        if (!pro_data) {
            return false;
        }
        int recv_event_id = -1;
        return pro_data->register_data_recv_event(robo_name, lchannel, nullptr, recv_event_id);
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}
bool hako_asset_check_data_recv_event(const char* asset_name, const char *robo_name, HakoPduChannelIdType lchannel)
{
    try {
        auto pro_data = hako::data::pro::hako_pro_get_data();
        if (!pro_data) {
            return false;
        }
        int recv_event_id;
        return pro_data->get_recv_event(asset_name, robo_name, lchannel, recv_event_id);    
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}



/*
 * for simevent
 */
bool hako_simevent_init()
{
    try {
        hako_simevent = hako::get_simevent_controller();
        if (hako_simevent == nullptr) {
            return false;
        }
        return true;
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

int hako_simevent_get_state()
{
    try {
        return (int)hako_simevent->state();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return -1;
    }
}

bool hako_simevent_start()
{
    try {
        return hako_simevent->start();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_simevent_stop()
{
    try {
        return hako_simevent->stop();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}

bool hako_simevent_reset()
{
    try {
        return hako_simevent->reset();
    } catch (std::exception *) {
        //hako::logger::get("core")->error(e->what());
        return false;
    }
}
