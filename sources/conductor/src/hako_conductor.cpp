#include "hako_conductor.h"
#include "hako_conductor_impl.hpp"

int hako_conductor_start(hako_time_t delta_usec, hako_time_t max_delay_usec)
{
    return hako_conductor_impl_start(delta_usec, max_delay_usec);
}

void hako_conductor_stop(void)
{
    hako_conductor_impl_stop();
}

