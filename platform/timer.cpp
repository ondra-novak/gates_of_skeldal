#include "timer.h"
#include <chrono>
#include "platform.h"

#include <thread>

int timerspeed_val = TIMERSPEED;

int get_timer_value() {
    auto n = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(n.time_since_epoch()).count()/timerspeed_val;
}

uint32_t get_game_tick_count() {
    auto n = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(n.time_since_epoch()).count();
}

void sleep_ms(uint32_t x) {
    std::this_thread::sleep_for(std::chrono::milliseconds(x));
}
