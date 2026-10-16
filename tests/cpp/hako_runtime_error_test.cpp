#include <gtest/gtest.h>

#include "hako_runtime_error_internal.hpp"
#include "utils/hako_share/hako_master_lock.hpp"

#include <atomic>
#include <stdexcept>
#include <thread>

namespace {

using hako::utils::sem::flock::MasterLockHealth;
using hako::utils::sem::flock::MasterLockStateError;

TEST(HakoRuntimeErrorTest, PreservesTypedMasterLockHealth)
{
    const int abort_result = hako::runtime::error::guard([]() -> int {
        throw MasterLockStateError(MasterLockHealth::Abort, "dead owner");
    }, -1);
    EXPECT_EQ(abort_result, -1);
    EXPECT_EQ(hako_runtime_last_error_status(), HAKO_RUNTIME_ERROR_ABORT);
    EXPECT_STREQ(hako_runtime_last_error(), "dead owner");

    const int unknown_result = hako::runtime::error::guard([]() -> int {
        throw MasterLockStateError(MasterLockHealth::Unknown, "unverifiable owner");
    }, -2);
    EXPECT_EQ(unknown_result, -2);
    EXPECT_EQ(hako_runtime_last_error_status(), HAKO_RUNTIME_ERROR_UNKNOWN);
    EXPECT_STREQ(hako_runtime_last_error(), "unverifiable owner");
}

TEST(HakoRuntimeErrorTest, OtherExceptionsRemainDiagnosable)
{
    EXPECT_FALSE(hako::runtime::error::guard([]() -> bool {
        throw std::runtime_error("ordinary failure");
    }, false));
    EXPECT_EQ(hako_runtime_last_error_status(), HAKO_RUNTIME_ERROR_OTHER);
    EXPECT_STREQ(hako_runtime_last_error(), "ordinary failure");
}

TEST(HakoRuntimeErrorTest, SuccessClearsEarlierFailure)
{
    (void)hako::runtime::error::guard([]() -> int {
        throw MasterLockStateError(MasterLockHealth::Abort, "dead owner");
    }, -1);
    ASSERT_EQ(hako_runtime_last_error_status(), HAKO_RUNTIME_ERROR_ABORT);

    EXPECT_EQ(hako::runtime::error::guard([]() { return 7; }, -1), 7);
    EXPECT_EQ(hako_runtime_last_error_status(), HAKO_RUNTIME_ERROR_NONE);
    EXPECT_STREQ(hako_runtime_last_error(), "");
}

TEST(HakoRuntimeErrorTest, DiagnosticsAreThreadLocal)
{
    hako_runtime_clear_error();
    std::atomic<bool> worker_ready{false};
    std::atomic<bool> release_worker{false};
    HakoRuntimeErrorStatus worker_status = HAKO_RUNTIME_ERROR_NONE;

    std::thread worker([&]() {
        (void)hako::runtime::error::guard([]() -> int {
            throw MasterLockStateError(MasterLockHealth::Unknown, "worker");
        }, -1);
        worker_ready.store(true, std::memory_order_release);
        while (!release_worker.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        worker_status = hako_runtime_last_error_status();
    });

    while (!worker_ready.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    EXPECT_EQ(hako_runtime_last_error_status(), HAKO_RUNTIME_ERROR_NONE);
    release_worker.store(true, std::memory_order_release);
    worker.join();
    EXPECT_EQ(worker_status, HAKO_RUNTIME_ERROR_UNKNOWN);
}

} // namespace

