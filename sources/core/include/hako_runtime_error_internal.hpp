#pragma once

#include "hako_runtime_error.h"

#include <utility>

namespace hako::runtime::error {

void capture_current_exception() noexcept;

template <typename Function, typename Return>
Return guard(Function&& function, Return failure) noexcept
{
    hako_runtime_clear_error();
    try {
        return std::forward<Function>(function)();
    }
    catch (...) {
        capture_current_exception();
        return failure;
    }
}

template <typename Function>
void guard_void(Function&& function) noexcept
{
    hako_runtime_clear_error();
    try {
        std::forward<Function>(function)();
    }
    catch (...) {
        capture_current_exception();
    }
}

} // namespace hako::runtime::error

