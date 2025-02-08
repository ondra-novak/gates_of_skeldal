#include <stdint.h>
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

char game_display_init(const INI_CONFIG_SECTION *display_section, const char *title);
void game_display_close(void);
void game_display_update_rect(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys);
char game_display_is_quit_requested();
void game_display_cancel_quit_request();
void game_display_set_icon(const void *icon_data, size_t icon_size);

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
