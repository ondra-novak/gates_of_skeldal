/*
 * input.h
 *
 *  Created on: 26. 1. 2025
 *      Author: ondra
 */

#ifndef PLATFORM_SDL_INPUT_H_
#define PLATFORM_SDL_INPUT_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <libs/devices.h>
#include "../config.h"


char get_control_key_state(void);
char get_shift_key_state(void);
char get_capslock_state(void);


uint32_t _bios_keybrd(int mode);
void SetWheelMapping(char up, char down);
void get_ms_event(MS_EVENT *event);
void ShareCPU();

void init_joystick(const INI_CONFIG_SECTION *section);
char is_joystick_used();

#ifdef __cplusplus
}
#endif


#endif /* PLATFORM_SDL_INPUT_H_ */
