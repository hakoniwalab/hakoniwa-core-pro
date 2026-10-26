#include <gtest/gtest.h>

#include <cerrno>
#include <sstream>
#include <string>

#include "hako_asset.h"
#include "hako_asset_state_check.hpp"

namespace {

TEST(HakoAssetStateCheckTest, StoppedStateIsAcceptedWithoutDiagnostics)
{
    std::ostringstream info;
    std::ostringstream error;

    EXPECT_EQ(
        hako::asset::internal::require_stopped_state(
            HakoSim_Stopped, info, error),
        0);
    EXPECT_TRUE(info.str().empty());
    EXPECT_TRUE(error.str().empty());
}

TEST(HakoAssetStateCheckTest, LifecycleTransitionsAreReportedAsExpectedRetry)
{
    for (const auto state : {HakoSim_Stopping, HakoSim_Resetting}) {
        std::ostringstream info;
        std::ostringstream error;

        EXPECT_EQ(
            hako::asset::internal::require_stopped_state(
                state, info, error),
            EINVAL);
        EXPECT_NE(
            info.str().find(
                "is transitioning; retry after it reaches HakoSim_Stopped."),
            std::string::npos);
        EXPECT_TRUE(error.str().empty());
    }
}

TEST(HakoAssetStateCheckTest, InvalidStateReportsTheObservedSnapshot)
{
    std::ostringstream info;
    std::ostringstream error;

    EXPECT_EQ(
        hako::asset::internal::require_stopped_state(
            HakoSim_Running, info, error),
        EINVAL);
    EXPECT_TRUE(info.str().empty());
    EXPECT_EQ(
        error.str(),
        "Error: simulation state(2) is invalid, expecting HakoSim_Stopped.\n");
}


TEST(HakoAssetStateCheckTest, PublicSimulationStateFollowsCoreValues)
{
    using hako::asset::internal::to_public_simulation_state;
    EXPECT_EQ(to_public_simulation_state(HakoSim_Stopped), HAKO_ASSET_SIM_STATE_STOPPED);
    EXPECT_EQ(to_public_simulation_state(HakoSim_Runnable), HAKO_ASSET_SIM_STATE_RUNNABLE);
    EXPECT_EQ(to_public_simulation_state(HakoSim_Running), HAKO_ASSET_SIM_STATE_RUNNING);
    EXPECT_EQ(to_public_simulation_state(HakoSim_Stopping), HAKO_ASSET_SIM_STATE_STOPPING);
    EXPECT_EQ(to_public_simulation_state(HakoSim_Resetting), HAKO_ASSET_SIM_STATE_RESETTING);
    EXPECT_EQ(to_public_simulation_state(HakoSim_Error), HAKO_ASSET_SIM_STATE_ERROR);
    EXPECT_EQ(to_public_simulation_state(HakoSim_Terminated), HAKO_ASSET_SIM_STATE_TERMINATED);
    EXPECT_EQ(to_public_simulation_state(HakoSim_Any), -1);
}

}  // namespace
