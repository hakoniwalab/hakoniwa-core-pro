#ifndef HAKO_ASSET_STATE_CHECK_HPP
#define HAKO_ASSET_STATE_CHECK_HPP

#include <cerrno>
#include <ostream>

#include "types/hako_types.hpp"

namespace hako::asset::internal {

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
