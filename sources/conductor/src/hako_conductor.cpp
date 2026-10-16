#include "hako_conductor.h"
#include "hako_conductor_impl.hpp"
#include "hako_runtime_error_internal.hpp"

int hako_conductor_start(hako_time_t delta_usec, hako_time_t max_delay_usec)
{
    return hako::runtime::error::guard([&]() {
        return hako_conductor_impl_start(delta_usec, max_delay_usec) ? 0 : -1;
    }, -1);
}

void hako_conductor_stop(void)
{
    hako::runtime::error::guard_void([]() { hako_conductor_impl_stop(); });
}

HakoRuntimeErrorStatus hako_conductor_last_error_status(void)
{
    return hako_runtime_last_error_status();
}

const char* hako_conductor_last_error(void)
{
    return hako_runtime_last_error();
}
