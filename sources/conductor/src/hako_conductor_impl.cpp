#include "hako_conductor_impl.hpp"
#include "hako_runtime_error_internal.hpp"
#include <chrono>
#include <cstdlib>
#include <thread>
#ifdef WIN32
#else
#include <unistd.h>
#endif

std::shared_ptr<hako::IHakoMasterController> hako_conductor = nullptr;
static bool hako_conductor_cmd_stop = false;
/*
 * When the world time cannot move (an asset is behind), the loop gives up its time slice
 * instead of spinning on: the conductor shares the process with the asset it serves (one
 * core busy all the time, 2026-10-17). HAKO_CONDUCTOR_IDLE_SLEEP_USEC > 0 sleeps that long
 * instead (less CPU, but each wait can be late by the OS's sleep granularity: on Windows a
 * short sleep often takes 1 ms or more, so the default stays at yield).
 */
static long hako_conductor_idle_sleep_usec()
{
    const char* value = std::getenv("HAKO_CONDUCTOR_IDLE_SLEEP_USEC");
    if (value == nullptr) {
        return 0;
    }
    char* end = nullptr;
    long usec = std::strtol(value, &end, 10);
    return (end != value && usec > 0) ? usec : 0;
}

void hako_conductor_impl_thread_run()
{
    std::cout << "INFO: hako_conductor thread start" << std::endl;
    const long idle_sleep_usec = hako_conductor_idle_sleep_usec();
    while (!hako_conductor_cmd_stop) {
        try {
            if (!hako_conductor->execute()) {
                if (idle_sleep_usec > 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(idle_sleep_usec));
                }
                else {
                    std::this_thread::yield();
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR: hako_conductor.execute() Failed: " << e.what() << std::endl;
            hako::runtime::error::capture_current_exception();
            hako_conductor = nullptr;
            return;
        }
    }
    hako_conductor = nullptr;
}

bool hako_conductor_impl_start(hako_time_t delta_usec, hako_time_t max_delay_usec)
{
    if (hako_conductor != nullptr) {
        return false;
    }

    try {
        hako::init();
        hako_conductor = hako::create_master();
        if (hako_conductor == nullptr) {
            std::cout << "INFO: hako::create_master() Failed" << std::endl;
            return false;
        }
        hako_conductor->set_config_simtime(max_delay_usec, delta_usec);
    } catch (std::exception& e) {
        std::cout << "INFO: hako::create_master() Failed: " << e.what() << std::endl;
        hako_conductor = nullptr;
        throw;
    }
    
    std::thread thread(hako_conductor_impl_thread_run);
    thread.detach();
    return true;
}
void hako_conductor_impl_stop(void)
{
    hako_conductor_cmd_stop = true;
    while (hako_conductor != nullptr) {
        usleep(HAKO_CONDUCTOR_WAIT_TIME_USEC);
    }
    return;
}
