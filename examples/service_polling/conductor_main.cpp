#include "hako_conductor.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace {
std::atomic_bool keep_running{true};
void stop_handler(int) { keep_running = false; }
}

// The conductor runs in its own process, like a Godot render server would see it.
int main()
{
    std::cout << std::unitbuf;  // keep logs in order when redirected
    std::signal(SIGINT, stop_handler);
    std::signal(SIGTERM, stop_handler);
    if (hako_conductor_start(1000, 1000) != 0) {
        std::cerr << "ERROR: conductor start failed\n";
        return 1;
    }
    std::cout << "CONDUCTOR started delta=1000us max_delay=1000us\n";
    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    hako_conductor_stop();
    return 0;
}
