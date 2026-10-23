#pragma once
#include <chrono>
#include <cstdint>

// steady_clock is mach_absolute_time on macOS and CLOCK_MONOTONIC on Linux, so the
// server and the client timestamps can be compared across processes.
inline int64_t bench_now_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
