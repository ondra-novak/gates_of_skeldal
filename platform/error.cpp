#include "legacy_coroutines.h"

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <iostream>
#include "error.h"
#include "platform.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


static std::uint32_t gtick = get_game_tick_count();
void send_log_impl(const char *format, ...) {
    va_list args;
    int task = q_current_task();
    char buff2[1000];
    char buff[1334];
    va_start(args, format);
    auto reltik = get_game_tick_count() - gtick;
    double sec = reltik * 0.001;
    vsnprintf(buff2,1000,format, args);
    va_end(args);
    snprintf(buff, sizeof(buff), "%f [%d] %s\r\n", sec, task, buff2);


    #ifdef _WIN32
        OutputDebugStringA(buff);
    #else
        std::cerr <<  buff;
    #endif

}

void throw_exception(const char *text) {
    throw std::runtime_error(std::string("Invoked crash:") + text);
}
