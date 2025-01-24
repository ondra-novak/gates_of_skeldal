#ifndef __BMOUSE_H
#define __BMOUSE_H

#include "event.h"
#include "devices.h"


#define get_mouse(info) ((MS_EVENT *)(*(long *) info->data))

extern MS_EVENT ms_last_event;

short init_mysky();
short done_mysky();
void ukaz_mysku();
void schovej_mysku();
void zobraz_mysku();
void set_ms_finger(int x,int y);
void update_mysky(void);
char je_myska_zobrazena();
#endif
