#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "config/hako_config.hpp"
#include "hako.hpp"
#include "utils/hako_share/hako_sem_flock.hpp"
#include "utils/hako_share/impl/hako_flock.hpp"
#include "utils/hako_config_loader.hpp"

int main(int argc, char **argv)
{
    if (!hako::init()) {
        std::cerr << "failed to initialize Hakoniwa master fixture" << std::endl;
        return 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--os-lock") {
        HakoConfigType config;
        hako_config_load(config);
        std::string path =
            config.param["core_mmap_path"].get<std::string>() + "/flock.bin";
        HakoFlockObjectType *handle = hako_flock_open(path);
        if (handle == nullptr) {
            return 2;
        }
        hako_flock_acquire(handle);
        std::cout << "READY" << std::endl;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    hako::utils::sem::flock::master_lock(HAKO_SHARED_MEMORY_ID_0);
    std::cout << "READY" << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
