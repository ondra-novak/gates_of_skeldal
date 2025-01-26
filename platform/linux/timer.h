/*
 * timer.h
 *
 *  Created on: 26. 1. 2025
 *      Author: ondra
 */

#ifndef PLATFORM_LINUX_TIMER_H_
#define PLATFORM_LINUX_TIMER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int get_timer_value();
uint32_t get_game_tick_count();
void sleep_ms(uint32_t);

#ifdef __cplusplus
}
#endif




#endif /* PLATFORM_LINUX_TIMER_H_ */
