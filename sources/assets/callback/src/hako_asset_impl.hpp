#ifndef _HAKO_ASSET_IMPL_HPP_
#define _HAKO_ASSET_IMPL_HPP_

#include "hako_asset.h"
#include "hako.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include "stdio.h"
#include "assert.h"
#include "hako_asset_pdu.hpp"

#define HAKO_ASSET_ASSERT(expr)	\
do {	\
	if (!(expr))	{	\
		printf("HAKO ASSERTION FAILED:%s:%s:%d:%s", __FILE__, __FUNCTION__, __LINE__, #expr);	\
		assert(!(expr));	\
	}	\
} while (0)

using json = nlohmann::json;

struct HakoAssetType {
    bool is_initialized;
    bool external_use;
    std::string asset_name_str;
    hako_time_t delta_usec;
    hako_time_t current_usec;
    json param;
    const hako_asset_callbacks_t *callback;
    std::shared_ptr<hako::IHakoAssetController> hako_asset;
    std::shared_ptr<hako::IHakoSimulationEventController> hako_sim;
    std::vector<hako::asset::Robot> robots;

#ifdef ENABLE_HAKO_TIME_MEASURE
    /*
     * for sim time measurement 
     */
    void* measure_vp;
#endif /* ENABLE_HAKO_TIME_MEASURE */
};

extern HakoAssetType hako_asset_instance;
#define HAKO_ASSET_WAIT_TIME_USEC (1000 * 10)

extern bool hako_asset_impl_register_callback(const hako_asset_callbacks_t* callback);
extern bool hako_asset_impl_init(const char* asset_name, const char* config_path, hako_time_t delta_usec, bool is_plant);
extern bool hako_asset_impl_wait_running(void);
extern HakoSimulationStateType hako_asset_impl_state();
extern bool hako_asset_impl_step(hako_time_t increment_step);
extern hako_time_t hako_asset_impl_get_world_time();
extern bool hako_asset_impl_pdu_read(const char* robo_name, HakoPduChannelIdType lchannel, char* buffer, size_t buffer_len);
extern bool hako_asset_impl_pdu_write(const char* robo_name, HakoPduChannelIdType lchannel, const char* buffer, size_t buffer_len);

extern bool hako_asset_impl_initialize_for_external();
extern int hako_asset_impl_pdu_create(const char *robo_name, HakoPduChannelIdType lchannel, size_t pdu_size);


/**
 * Register a data receive event for a specific logical channel.
 *
 * @param robo_name The name of the robot (virtual asset).
 * @param lchannel The logical PDU channel ID.
 * @param on_recv Callback function to be invoked when data is received.
 *                If NULL, a flag-based mechanism will be used instead.
 * @return true if the registration succeeded, false otherwise.
 */
extern bool hako_asset_impl_register_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel, void (*on_recv)());

/**
 * Check if a receive event has occurred (only valid when flag-based).
 *
 * @param robo_name The name of the robot (virtual asset).
 * @param lchannel The logical PDU channel ID.
 * @return true if a receive event was detected, false otherwise.
 */
extern bool hako_asset_impl_check_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel);


/*
 * for master api for test
 */
extern bool hako_master_impl_start(hako_time_t delta_usec, hako_time_t max_delay_usec);
extern void hako_master_impl_stop(void);

#endif /* _HAKO_ASSET_IMPL_HPP_ */
