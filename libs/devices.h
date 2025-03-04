#ifndef __DEVICES_H
#define __DEVICES_H

#include "types.h"
#include "event.h"
#include "mouse.h"

typedef struct tms_basic_info
  {
        int mouse_event;
        unsigned short mouse_code;
        unsigned short mouse_bx;
        unsigned short mouse_cx;
        unsigned short mouse_dx;
        signed short mouse_si;
        signed short mouse_di;
  }TMS_BASIC_INFO;

extern TMS_BASIC_INFO ms_basic_info;

int lock_region (void *address, unsigned length);
void keyboard(EVENT_MSG *msg,void *user_data);
char ms_get_keycount(void);

#endif
