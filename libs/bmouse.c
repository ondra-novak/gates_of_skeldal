#include <platform/platform.h>
#include "types.h"
#include "bgraph.h"
#include "event.h"
#include "devices.h"
#include "bmouse.h"

#include "memman.h"

#include <stdio.h>

char visible=0;
MS_EVENT ms_last_event;
integer h_x,h_y=0;

static const void *cur_hw_mouse = NULL;

void ukaz_mysku()
  {
  if (!visible) return;
  visible--;
  if (!visible)
     {
#ifdef FORCE_SOFTWARE_CURSOR
     show_ms_cursor(ms_last_event.x-h_x,ms_last_event.y-h_y);
#else
     const void *reg = (const word *)get_registered_ms_cursor();
     if (reg != cur_hw_mouse) {
         cur_hw_mouse = reg;
         game_display_show_mouse(reg,h_x, h_y);
     }
#endif
     }
  }

void schovej_mysku()
  {
#ifdef FORCE_SOFTWARE_CURSOR
  if (!visible)
     hide_ms_cursor();
#endif
  visible++;
  }

void zobraz_mysku()
  {
  visible=1;
  ukaz_mysku();
  }

void ms_idle_event(EVENT_MSG *info,void *user_data)
  {
  void *i;MS_EVENT x;
  user_data;info;
  if (info->msg==E_WATCH)
     {
     *otevri_zavoru=1;
     get_ms_event(&x);
     if (x.event)
       {
       ms_last_event=x;
       i=&ms_last_event;
       *otevri_zavoru=1;
       send_message(E_MOUSE,i);
       }
     }
  }

void ms_draw_event(EVENT_MSG *info,void *user_data)
  {
#ifdef FORCE_SOFTWARE_CURSOR
  MS_EVENT *ms_ev;

  user_data;
  if (info->msg==E_MOUSE)
     {
     ms_ev=get_mouse(info);
     if (ms_ev->event_type & 1)
       if (!visible) move_ms_cursor(ms_ev->x-h_x,ms_ev->y-h_y,0);
     }
#endif
  }


void update_mysky(void)
  {
#ifdef FORCE_SOFTWARE_CURSOR
  MS_EVENT x;

  get_ms_event(&x);
  if (x.event)
     {
     ms_last_event=x;
     x.event = 0;
     }
  if(!visible) move_ms_cursor(x.x-h_x,x.y-h_y,0);
#endif
  }

char je_myska_zobrazena()
  {
  return !visible;
  }


void set_ms_finger(int x,int y)
  {
  h_x=x;
  h_y=y;
  }


short init_mysky()
  {

//  i=install_mouse_handler();
//  hranice_mysky(0,0,639,479);
  visible=1;
  send_message(E_ADD,E_WATCH,ms_idle_event);
 #ifdef FORCE_SOFTWARE_CURSOR
  send_message(E_ADD,E_MOUSE,ms_draw_event);
 #endif
  return 0;
  }

short done_mysky()
  {

//  i=deinstall_mouse_handler();
  send_message(E_DONE,E_WATCH,ms_idle_event);
#ifdef FORCE_SOFTWARE_CURSOR
  send_message(E_DONE,E_MOUSE,ms_draw_event);
#endif
  return 0;
  }

