#pragma once

#include "sdl_context.h"

inline SDLContext &get_sdl_global_context () {
    static SDLContext sdl_global_context;
    return sdl_global_context;
}



