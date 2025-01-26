#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __SKELDAL__MOUSE__
#define __SKELDAL__MOUSE__
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
