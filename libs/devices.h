#ifndef __DEVICES_H
#define __DEVICES_H

#include "types.h"
#include "event.h"
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

typedef struct ms_event
  {
   char event;
   word x,y;
   char tl1,tl2,tl3;
   word event_type;
  }MS_EVENT;

extern TMS_BASIC_INFO ms_basic_info;
extern char ms_fake_mode;

//int install_mouse_handler();
//int deinstall_mouse_handler();
//void hranice_mysky(int x1,int y1,int x2,int y2);
void get_ms_event(MS_EVENT *event);
int lock_region (void *address, unsigned length);
void keyboard(EVENT_MSG *msg,void *user_data);
char ms_get_keycount();
#endif
