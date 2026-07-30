#include <gtest/gtest.h>

#include <cerrno>
#include <sstream>
#include <string>

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

}  // namespace
