#include "global_context.h"
#include "input.h"
#include "../platform.h"

char get_control_key_state() {
    return get_sdl_global_context().get_control_key_state()?1:0;
}
char get_shift_key_state() {
    return get_sdl_global_context().get_shift_key_state()?1:0;
}
char get_capslock_state() {
    return get_sdl_global_context().get_capslock_state()?1:0;
}
uint32_t _bios_keybrd(int mode) {
    if (mode == _KEYBRD_READY) {
        return get_sdl_global_context().is_keyboard_ready()?1:0;
    } else if (mode == _KEYBRD_READ) {
        return get_sdl_global_context().pop_keyboard_code();
    } else {
        return 0;
    }

}

void SetWheelMapping(char up, char down) { //todo

}



void get_ms_event(MS_EVENT *event) {
    *event = get_sdl_global_context().getMsEvent();

}

void ShareCPU() {
    if (q_is_mastertask()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}
