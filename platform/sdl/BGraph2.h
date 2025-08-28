#include <stdint.h>
#include <stdarg.h>
#include "../config.h"

#include <stddef.h>
#ifndef __BGRAPH_DX_WRAPPER_
#define __BGRAPH_DX_WRAPPER_

#ifdef __cplusplus
extern "C" {
#endif

uint16_t *GetScreenAdr(void);
uint16_t *GetBuffer2nd(void);
int32_t GetScreenPitch(void);
int32_t GetBuffer2ndPitch(void);
int32_t GetScreenSizeBytes(void);

void RedirectScreen(uint16_t *newaddr);
void RestoreScreen(void);
void RedirectScreenBufferSecond(void);


///Initializes display - in current thread (main thread), starts game thread. Display is closed when thread finishes
int game_display_init(const INI_CONFIG_SECTION *display_section,
            const char *title,
            int (*game_thread)(va_list), ...);
void game_display_update_rect(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys);
char game_display_is_quit_requested();
void game_display_cancel_quit_request();
void game_display_set_icon(const void *icon_data, size_t icon_size);
void game_display_show_mouse(const unsigned short *mouse_image, int finger_x, int finger_y);
void game_display_hide_mouse();
///Load sprite HI format, sprite_id can be any integer value- if sprite exists it is replaced
void game_display_load_sprite(int sprite_id, const unsigned short *hi_image);
///show and place sprite at given coordinates
void game_display_place_sprite(int sprite_id, int x, int y);
///show and place (and scale) sprite at given coordinates
void game_display_scale_sprite(int sprite_id, int x, int y, int w, int h);
void game_display_sprite_set_zindex(int sprite_id, int zindex);
///hide sprite
void game_display_hide_sprite(int sprite_id);
///unload sprite and free index
void game_display_unload_sprite(int sprite);
void game_display_disable_crt_effect_temporary(char disable);

void *DxPrepareWalk(int ypos);
void DxZoomWalk(void *handle, int ypos, int *points,float phase, void *lodka);
void DxDoneWalk(void *handle);

void *DxPrepareTurn(int ypos);
//phase > 0 right, phase < 0 left
void DxTurn(void *handle, int ypos,int border, float phase, void *lodka);
void DxDoneTurn(void *handle);
void DxTurnLeftRight(char right, float phase, int border, int ypos, int *last);


void DxDialogs(char enable);

void setvesa_displaystart(int x,int y);


void DxSetInitResolution(int x, int y);
int DxGetResX(void);
int DxGetResY(void);

void DXMouseTransform(unsigned short *x, unsigned short *y);

//HWND GetGameWindow();
//void DxLockBuffers(BOOL lock);

void StripBlt(const void *data, unsigned int startline, uint32_t width);


#ifdef __cplusplus
  }
#endif


#endif
