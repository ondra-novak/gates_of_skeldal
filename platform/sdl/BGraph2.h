#include <stdint.h>

#ifndef __BGRAPH_DX_WRAPPER_
#define __BGRAPH_DX_WRAPPER_

#ifdef __cplusplus
extern "C" {
#endif

uint16_t *GetScreenAdr();
uint16_t *GetBuffer2nd();
int32_t GetScreenPitch();
int32_t GetBuffer2ndPitch();
int32_t GetScreenSizeBytes();

void RedirectScreen(uint16_t *newaddr);
void RestoreScreen();
void RedirectScreenBufferSecond();


//inicializuje a otevira rezim 640x480x16b v DX - otevre okno, pripravi vse pro beh hry
//Vraci 1 pri uspechu
char DXInit64(char inwindow,int zoom,int monitor, int refresh);

//uzavre rezim grafiky
void DXCloseMode();

//void DXCopyRects32(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys);
void DXCopyRects64(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys);

void *DxPrepareWalk(int ypos);
void DxZoomWalk(void *handle, int ypos, int *points,float phase, void *lodka);
void DxDoneWalk(void *handle);

void *DxPrepareTurn(int ypos);
void DxTurn(void *handle, char right, int ypos,int border, float phase, void *lodka);
void DxDoneTurn(void *handle);
void DxTurnLeftRight(char right, float phase, int border, int ypos, int *last);


void DxDialogs(char enable);

void setvesa_displaystart(int x,int y);


void DxSetInitResolution(int x, int y);
int DxGetResX();
int DxGetResY();

void DXMouseTransform(unsigned short *x, unsigned short *y);

//HWND GetGameWindow();
//void DxLockBuffers(BOOL lock);

void StripBlt(void *data, unsigned int startline, uint32_t width);


#ifdef __cplusplus
  }
#endif


#endif
