#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifndef _SKELDAL_PLATFORM_HEADER_
#define _SKELDAL_PLATFORM_HEADER_

#define BGSWITCHBIT 0x8000


#define SKELDALINI "skeldal.ini"

#ifdef __cplusplus
extern "C"
  {
#endif


#define _KEYBRD_READY 0
#define _KEYBRD_READ 1

#define TIMERSPEED 20

uint32_t _bios_keybrd(int mode);


#define RGB888(r,g,b) ((unsigned short)((((r)<<7)&0x7C00) | (((g)<<2) & 0x3E0) | ((b)>>3)))
#define RGB555(r,g,b) (((unsigned short)(((r)<<10) | ((g)<<5) | (b))) & ~BGSWITCHBIT)
#define RGB555_ALPHA(r,g,b) (((unsigned short)(((r)<<10) | ((g)<<5) | (b))) | BGSWITCHBIT)



///build pathname from parts
/**
 * @param nparts count of parts (must be > 0)
 * @param part first part
 * @return complete path - statically allocated buffer
 */
const char * build_pathname(size_t nparts, const char *part1, ...);
///creates directories recursively
/**
 * @param path path
 * @retval 1 success
 * @retval 0 failure
 */
char create_directories(const char *path);
///change current directory
/**
 * @param path path
 * @retval 1 success
 * @retval 0 failure
 */
char change_current_directory(const char *path);



void *map_file_to_memory(const char *name, size_t *sz);
void unmap_file(void *ptr, size_t sz);

void ShareCPU(void);
void SetWheelMapping(char up, char down);

char get_control_key_state(void);
char get_shift_key_state(void);
char get_capslock_state(void);
void display_error(const char *text);
///returns -1 if doesn't exists
char check_file_exists(const char *pathname);
FILE *fopen_icase(const char *pathname, const char *mode);
const char *file_icase_find(const char *pathname);

int stricmp(const char *a, const char *b);
#define MIN(a, b) ((a)<(b)?(a):(b))
#define MAX(a, b) ((a)>(b)?(a):(b))
void strupr(char *c);
const char * int2ascii(int i, char *c, int radix);

int get_timer_value(void);
uint32_t get_game_tick_count(void);
void sleep_ms(uint32_t);

#ifdef __cplusplus
  }
#endif


#include "sdl/BGraph2.h"

#define WM_RELOADMAP (WM_APP+215)
#define E_RELOADMAP 40

typedef struct _ReloadMapInfo {
        const char *fname;
        int sektor;
   } ReloadMapInfo;



#endif
