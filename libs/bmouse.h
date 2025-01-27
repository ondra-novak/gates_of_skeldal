#ifndef __BMOUSE_H
#define __BMOUSE_H

#include "event.h"
#include "devices.h"


#define get_mouse(info) va_arg(info->data,MS_EVENT *)

extern MS_EVENT ms_last_event;

short init_mysky(void);
short done_mysky(void);
void ukaz_mysku(void);
void schovej_mysku(void);
void zobraz_mysku(void);
void set_ms_finger(int x,int y);
void update_mysky(void);
char je_myska_zobrazena(void);
#endif
