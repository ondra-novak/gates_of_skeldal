#include "global_context.h"
#include "input.h"

char get_control_key_state() {
    return 0;   //todo
}
char get_shift_key_state() {
    return 0;   //todo
}
uint32_t _bios_keybrd(int mode) {
    return 0;
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
