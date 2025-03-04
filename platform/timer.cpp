#include "timer.h"
#include <chrono>
#include "platform.h"

#include <thread>

int timerspeed_val = TIMERSPEED;

static auto start_tm =  std::chrono::steady_clock::now();;

int get_timer_value() {
    auto n = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(n-start_tm).count()/timerspeed_val;
}

uint32_t get_game_tick_count() {
    auto n = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(n-start_tm).count();
}

void sleep_ms(uint32_t x) {
    std::this_thread::sleep_for(std::chrono::milliseconds(x));
}
