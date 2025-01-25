#include <stdint.h>

#define BGSWITCHBIT 0x0020

#define SKELDALINI "WSKELDAL.INI"

#ifdef __cplusplus
extern "C"
  {
#endif


#define _KEYBRD_READY 0
#define _KEYBRD_READ 1

#define TIMERSPEED 20;

unsigned long _bios_keybrd(int mode);


//LRESULT GameMainWindowWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


void CheckMessageQueue();

//void DSReportWindowCreation(HWND hWindow);
char *AutodetectWinAmp();

#ifdef __cplusplus
  }
#endif

#define RGB888(r,g,b) ((unsigned short)((((r)<<8)&0xF800) | (((g)<<3) & 0x7C0) | ((b)>>3)))
#define RGB555(r,g,b) ((unsigned short)(((r)<<11) | ((g)<<6) | (b)))

#pragma warning (disable : 4244 4761 4133)


void *LoadDefaultFont();
void *LoadResourceFont(const char *name);

void ShareCPU();
void SetWheelMapping(char up, char down);

char get_control_key_state();
char get_shift_key_state();
void display_error(const char *text);

int stricmp(const char *a, const char *b);
#define MIN(a, b) ((a)<(b)?(a):(b))
#define MAX(a, b) ((a)>(b)?(a):(b))
void strupr(char *c);
const char * itoa(int i, char *c, int radix);

uint32_t get_game_tick_count();
void sleep_ms(uint32_t);

//------------- BGRAPH DX wrapper -------------------
#include "BGraph2Dx.h"

#define WM_RELOADMAP (WM_APP+215)
#define E_RELOADMAP 40

typedef struct _ReloadMapInfo {
        const char *fname;
        int sektor;
   } ReloadMapInfo;


