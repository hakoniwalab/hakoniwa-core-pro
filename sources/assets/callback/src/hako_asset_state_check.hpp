#ifndef HAKO_ASSET_STATE_CHECK_HPP
#define HAKO_ASSET_STATE_CHECK_HPP

#include <cerrno>
#include <ostream>

#include "types/hako_types.hpp"

namespace hako::asset::internal {

// The public HakoAssetSimulationStateType (hako_asset.h) uses Core's values.
static_assert(HakoSim_Stopped == 0 && HakoSim_Runnable == 1 && HakoSim_Running == 2
                  && HakoSim_Stopping == 3 && HakoSim_Resetting == 4
                  && HakoSim_Error == 5 && HakoSim_Terminated == 6,
              "HakoAssetSimulationStateType in hako_asset.h must follow HakoSimulationStateType");

/* Core state to the public value; -1 for a value the public API does not define. */
inline int to_public_simulation_state(HakoSimulationStateType state)
{
    switch (state) {
    case HakoSim_Stopped:
    case HakoSim_Runnable:
    case HakoSim_Running:
    case HakoSim_Stopping:
    case HakoSim_Resetting:
    case HakoSim_Error:
    case HakoSim_Terminated:
        return static_cast<int>(state);
    default:
        return -1;
    }
}

inline int require_stopped_state(
    HakoSimulationStateType state,
    std::ostream& info,
    std::ostream& error)
{
    if (state == HakoSim_Stopped) {
        return 0;
    }
    if (state == HakoSim_Stopping || state == HakoSim_Resetting) {
        info << "INFO: simulation state(" << state
             << ") is transitioning; retry after it reaches HakoSim_Stopped."
             << std::endl;
        return EINVAL;
    }
    error << "Error: simulation state(" << state
          << ") is invalid, expecting HakoSim_Stopped."
          << std::endl;
    return EINVAL;
}

}  // namespace hako::asset::internal

#endif  // HAKO_ASSET_STATE_CHECK_HPP
