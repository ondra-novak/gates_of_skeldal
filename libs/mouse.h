#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __SKELDAL__MOUSE__
#define __SKELDAL__MOUSE__

#define MS_EVENT_MOUSE_MOVE 1
#define MS_EVENT_MOUSE_LPRESS 2
#define MS_EVENT_MOUSE_LRELEASE 4
#define MS_EVENT_MOUSE_RPRESS 8
#define MS_EVENT_MOUSE_RRELEASE 16
#define MS_EVENT_MOUSE_MPRESS 32
#define MS_EVENT_MOUSE_MRELEASE 64


typedef struct ms_event
  {
   char event;
   uint16_t x,y;
   char tl1,tl2,tl3;
   uint16_t event_type;
  }MS_EVENT;


void get_ms_event(MS_EVENT *event);

#endif

#ifdef __cplusplus
}
#endif
