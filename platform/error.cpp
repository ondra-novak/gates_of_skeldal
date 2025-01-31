#include "legacy_coroutines.h"

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <iostream>
#include "error.h"
#include "platform.h"


void display_error(const char *text) {
    std::cerr << "ERROR:" << text << std::endl;
    abort();
}


static std::uint32_t gtick = get_game_tick_count();
void send_log_impl(const char *format, ...) {
    va_list args;
    int task = q_current_task();
    char buff2[1000];
    va_start(args, format);
    auto reltik = get_game_tick_count() - gtick;
    double sec = reltik * 0.001;
    std::cerr << sec << "[" << task << "]";
    vsnprintf(buff2,1000,format, args);
    std::cerr << buff2 << std::endl;
    va_end(args);


}
