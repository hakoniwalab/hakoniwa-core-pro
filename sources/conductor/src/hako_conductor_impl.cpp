#include "hako_conductor_impl.hpp"
#include <thread>
#ifdef WIN32
#else
#include <unistd.h>
#endif

std::shared_ptr<hako::IHakoMasterController> hako_conductor = nullptr;
static bool hako_conductor_cmd_stop = false;
void hako_conductor_impl_thread_run()
{
    std::cout << "INFO: hako_conductor thread start" << std::endl;
    while (!hako_conductor_cmd_stop) {
        try {
            hako_conductor->execute();
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR: hako_conductor.execute() Failed: " << e.what() << std::endl;
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
        return false;
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
