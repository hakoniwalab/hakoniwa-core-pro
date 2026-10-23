#pragma once
// Shared part of the servers that register as polling assets: start/stop/reset
// feedback and the asset time. The service itself is handled by the caller.
#include "hakoniwa_asset_polling.h"

#include <functional>
#include <iostream>

struct PollingAssetLoop {
    static constexpr int kEventStart = 1;
    static constexpr int kEventStop = 2;
    static constexpr int kEventReset = 3;
    static constexpr int kEventError = 4;
    static constexpr int kStateRunning = 2;

    const char* asset_name;
    hako_time_t delta_usec;
    hako_time_t local_time = 0;

    // Handles one asset event. on_start runs before the start feedback, so the
    // simulation reaches Running only after the server exists.
    // Returns false on an error event.
    bool handle_event(const std::function<bool()>& on_start)
    {
        const int event = hakoniwa_asset_get_event(asset_name);
        if (event == kEventStart) {
            if (!hakoniwa_asset_is_pdu_created() || !on_start()) {
                std::cerr << "ERROR: start handling failed\n";
                return false;
            }
            local_time = 0;
            hakoniwa_asset_notify_write_pdu_done(asset_name);
            hakoniwa_asset_start_feedback_ok(asset_name);
            std::cout << "SERVER event Start\n";
        } else if (event == kEventStop) {
            hakoniwa_asset_stop_feedback_ok(asset_name);
            std::cout << "SERVER event Stop\n";
        } else if (event == kEventReset) {
            local_time = 0;
            hakoniwa_asset_reset_feedback_ok(asset_name);
            std::cout << "SERVER event Reset\n";
        } else if (event == kEventError) {
            return false;
        }
        return true;
    }

    bool running() const
    {
        return hakoniwa_simevent_get_state() == kStateRunning;
    }

    // Advances the asset time by one step when the world time allows it.
    void advance_time()
    {
        if (local_time <= hakoniwa_asset_get_worldtime()) {
            local_time += delta_usec;
            hakoniwa_asset_notify_simtime(asset_name, local_time);
        }
    }
};
