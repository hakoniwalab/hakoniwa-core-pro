#include "hako_runtime_error_internal.hpp"

#include "utils/hako_share/hako_master_lock.hpp"

#include <exception>
#include <string>

namespace {

thread_local HakoRuntimeErrorStatus runtime_error_status =
    HAKO_RUNTIME_ERROR_NONE;
thread_local std::string runtime_error_message;

void set_error(HakoRuntimeErrorStatus status, const char *message) noexcept
{
    runtime_error_status = status;
    try {
        runtime_error_message = message == nullptr ? "" : message;
    }
    catch (...) {
        runtime_error_message.clear();
    }
}

} // namespace

HakoRuntimeErrorStatus hako_runtime_last_error_status(void)
{
    return runtime_error_status;
}

const char* hako_runtime_last_error(void)
{
    return runtime_error_message.c_str();
}

const char* hako_runtime_error_status_name(HakoRuntimeErrorStatus status)
{
    switch (status) {
    case HAKO_RUNTIME_ERROR_NONE: return "NONE";
    case HAKO_RUNTIME_ERROR_ABORT: return "ABORT";
    case HAKO_RUNTIME_ERROR_UNKNOWN: return "UNKNOWN";
    case HAKO_RUNTIME_ERROR_OTHER: return "OTHER";
    }
    return "OTHER";
}

void hako_runtime_clear_error(void)
{
    runtime_error_status = HAKO_RUNTIME_ERROR_NONE;
    runtime_error_message.clear();
}

void hako::runtime::error::capture_current_exception() noexcept
{
    try {
        throw;
    }
    catch (const hako::utils::sem::flock::MasterLockStateError& error) {
        using hako::utils::sem::flock::MasterLockHealth;
        switch (error.health()) {
        case MasterLockHealth::Abort:
            set_error(HAKO_RUNTIME_ERROR_ABORT, error.what());
            break;
        case MasterLockHealth::Unknown:
            set_error(HAKO_RUNTIME_ERROR_UNKNOWN, error.what());
            break;
        case MasterLockHealth::Ok:
            set_error(HAKO_RUNTIME_ERROR_OTHER, error.what());
            break;
        }
    }
    catch (const std::exception& error) {
        set_error(HAKO_RUNTIME_ERROR_OTHER, error.what());
    }
    catch (...) {
        set_error(HAKO_RUNTIME_ERROR_OTHER, "unknown native exception");
    }
}

