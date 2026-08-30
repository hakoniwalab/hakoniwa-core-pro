#include <chrono>
#include <iostream>
#include <thread>

#include "config/hako_config.hpp"
#include "hako.hpp"
#include "utils/hako_share/hako_sem_flock.hpp"

int main()
{
    if (!hako::init()) {
        std::cerr << "failed to initialize Hakoniwa master fixture" << std::endl;
        return 1;
    }
    hako::utils::sem::flock::master_lock(HAKO_SHARED_MEMORY_ID_0);
    std::cout << "READY" << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
