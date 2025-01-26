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

static MS_EVENT ms_event  = {};



void get_ms_event(MS_EVENT *event) {
    *event = ms_event;
}

void ShareCPU() {

}
