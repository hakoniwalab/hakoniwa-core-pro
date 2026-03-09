#pragma once

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace hako::profile {

inline bool enabled()
{
    static int cached = -1;
    if (cached < 0) {
        const char* env = std::getenv("HAKO_PROFILE_SERVICE_CLIENT");
        cached = (env != nullptr && *env != '\0' && *env != '0') ? 1 : 0;
    }
    return cached == 1;
}

class ScopedTimer {
public:
    explicit ScopedTimer(std::string label)
        : label_(std::move(label))
        , start_(std::chrono::steady_clock::now())
        , active_(enabled())
    {}

    ~ScopedTimer()
    {
        if (!active_) {
            return;
        }
        const auto end = std::chrono::steady_clock::now();
        const auto usec = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << "PROFILE: " << label_ << " usec=" << usec << std::endl;
    }

private:
    std::string label_;
    std::chrono::steady_clock::time_point start_;
    bool active_;
};

} // namespace hako::profile

