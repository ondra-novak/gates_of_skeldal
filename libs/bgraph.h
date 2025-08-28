#include <platform/platform.h>
#include "types.h"




word *GetScreenAdr(void);
word *GetBuffer2nd(void);
int32_t GetScreenPitch(void);
int32_t GetBuffer2ndPitch(void);
int32_t GetScreenSizeBytes(void);

void RedirectScreen(word *newaddr);
void RestoreScreen(void);
void RedirectScreenBufferSecond(void);


extern word curcolor,charcolors[7];
extern const word *curfont;
extern word *writepos,writeposx;
extern byte fontdsize;
extern void (*showview)(word,word,word,word);
extern char __skip_change_line_test;
extern char no_restore_mode;

static __inline word *getadr32(longint x,longint y)
  {
  return GetScreenAdr()+GetScreenPitch()*y+x;
  }

static __inline void point32(longint x,longint y, word color)
  {
  *getadr32(x,y)=color;
  }
void bar32(int x1,int y1, int x2, int y2);
//#pragma aux bar32 parm [eAX] [eBX] [eCX] [eDX] modify [ESI EDI];
void hor_line32(int x1,int y1,int x2);
//#pragma aux hor_line32 parm [eSi] [eAX] [eCX] modify [eDI eDX];
void ver_line32(int x1,int y1,int y2);
//#pragma aux ver_line32 parm [eSi] [eAX] [eCX] modify [eDX];
void hor_line_xor(int x1,int y1,int x2);
//#pragma aux hor_line_xor parm [eSi] [eAX] [eCX] modify [eDI eDX];
void ver_line_xor(int x1,int y1,int y2);
//#pragma aux ver_line_xor parm [eSi] [eAX] [eCX] modify [eDX];
void line_32(int x,int y,int xs,int ys);
//#pragma aux line_32 parm [esi] [eax] [ecx] [ebx] modify [edx edi]
void char_32(word *posit,const word *font,char znak);
//#pragma aux char_32 parm [edi] [esi] [eax] modify [eax ebx ecx edx]
void char2_32(word *posit,const word *font,char znak);
//#pragma aux char2_32 parm [edi] [esi] [eax] modify [eax ebx ecx edx]
word charsize(const word *font,char znak);
//#pragma aux charsize parm [esi] [eax]
void put_picture(word x,word y,const void *p);

void put_picture_ex(word x,word y,const void *p, word *target_addr, size_t pitch);
//#pragma aux put_picture parm [esi] [eax] [edi] modify [ebx ecx edx]
void get_picture(word x,word y,word xs,word ys,void *p);
//#pragma aux get_picture parm [esi] [eax] [ebx] [ecx] [edi] modify [edx]
void setpal(void *paleta);
//#pragma aux setpal parm [esi] modify [eax edx]
void redraw_lo(void *screen,void *lbuffer,byte *xlat);
//#pragma aux redraw_lo parm [esi][edi][ebx] modify[eax ecx edx]
void redraw256(void *screen,void *lbuffer,byte *xlat);
//#pragma aux redraw256 parm [esi][edi][ebx] modify [eax ecx edx]
void redraw256b(void *screen,void *lbuffer,byte *xlat);
//#pragma aux redraw256b parm [esi][edi][ebx] modify [eax ecx edx]
void redraw32(void *screen,void *lbuffer,byte *xlat);
//#pragma aux redraw32 parm [esi][edi][ebx] modify [ecx]
void redraw32b(void *screen,void *lbuffer,byte *xlat);
//#pragma aux redraw32b parm [esi][edi][ebx] modify [ecx eax]
void redraw64(void *screen,void *lbuffer,byte *xlat);
//#pragma aux redraw64 parm [esi][edi][ebx] modify [ecx eax]
void redraw64b(void *screen,void *lbuffer,byte *xlat);
//#pragma aux redraw64b parm [esi][edi][ebx] modify [ecx eax]
void redraw32bb(void *screen,void *lbuffer,byte *xlat);
//#pragma aux redraw32bb parm [esi][edi][ebx] modify [ecx eax]
void redrawbox_lo(word xs,word ys,void *screen,void *lbuffer,byte *xlat);
//#pragma aux redrawbox_lo parm [ecx][edx][esi][edi][ebx] modify [eax edx]
void redrawbox256(word xs,word ys,void *screen,void *lbuffer,byte *xlat);
//#pragma aux redrawbox256 parm [edx][ecx][esi][edi][ebx] modify [eax edx]
void redrawbox256b(word xs,word ys,void *screen,void *lbuffer,byte *xlat);
//#pragma aux redrawbox256b parm [edx][ecx][esi][edi][ebx] modify [eax edx]
void redrawbox32(word xs,word ys,void *screen,void *lbuffer);
//#pragma aux redrawbox32 parm [ebx][edx][esi][edi] modify [ecx eax]
void redrawbox32b(word xs,word ys,void *screen,void *lbuffer);
//#pragma aux redrawbox32b parm [ebx][edx][esi][edi] modify [ecx eax]
void redrawbox64(word xs,word ys,void *screen,void *lbuffer,byte *xlat);
//#pragma aux redrawbox64 parm [ecx][edx][esi][edi][ebx] modify [eax]
void redrawbox64b(word xs,word ys,void *screen,void *lbuffer,byte *xlat);
//#pragma aux redrawbox64b parm [ecx][edx][esi][edi][ebx]modify [eax]
void redrawbox32bb(word xs,word ys,void *screen,void *lbuffer);
//#pragma aux redrawbox32bb parm [ebx][edx][esi][edi] modify [ecx]
void redraw16(void *screen,void *lbuffer,byte *xlat);
void redrawbox16(word xs,word ys,void *screen,void *lbuffer,byte *xlat);
//#pragma aux redrawbox16 parm [edx][ecx][esi][edi][ebx] modify [eax edx]
//#pragma aux redraw16 parm [esi][edi][ebx] modify [ecx]
void showview32(word x,word y,word xs,word ys);
void showview256(word x,word y,word xs,word ys);
void showview_lo(word x,word y,word xs,word ys);
void outtext(const char *text);
void outtext_ex(const char *text, int space);
void outtext_w_nl(const char *text);
void line32(word x1,word y1, word x2, word y2);
void position(word x,word y);
void show_ms_cursor(integer x,integer y);
void *register_ms_cursor(const void *cursor);
const void *get_registered_ms_cursor();
void move_ms_cursor(integer newx,integer newy,char nodraw);
void hide_ms_cursor(void);
void redraw_ms_cursor_on_screen(void);
int text_height(  const char *text);
int text_width(  const char *text);
void set_aligned_position(int x,int y,char alignx, char aligny,const char *text);
void rectangle(int x1,int y1,int x2,int y2,int color);
word *mapvesaadr1(word *a);
void rel_position_x(word x);

void put_8bit_clipped(const void *src,void *trg,int startline,int velx,int vely);
//#pragma aux put_8bit_clipped parm [ESI][EDI][EAX][EBX][EDX] modify [ECX];
void put_textured_bar_(const void *src,void *trg,int xsiz,int ysiz,int xofs,int yofs);
//#pragma aux put_textured_bar_ parm [EBX][EDI][EDX][ECX][ESI][EAX];
void put_textured_bar(const void *src,int x,int y,int xs,int ys,int xofs,int yofs);
void trans_bar(int x,int y,int xs,int ys,int barva);
//#pragma aux trans_bar parm [EDI][ESI][EDX][ECX][EBX] modify [EAX];
void trans_bar25(int x,int y,int xs,int ys);
//#pragma aux trans_bar25 parm [EDI][ESI][EDX][ECX] modify [EAX EBX];
void trans_line_x(int x,int y,int xs,int barva);
//#pragma aux trans_line_x parm [EDI][ESI][ECX][EDX] modify [EAX];
void trans_line_y(int x,int y,int ys,int barva);
//#pragma aux trans_line_y parm [EDI][ESI][ECX][EDX] modify [EAX];
void draw_placed_texture(const short *txtr,int celx,int cely,int posx,int posy,int posz,char turn);

void put_image(const word *image,word *target,int start_line,int sizex,int sizey);
//#pragma aux put_image parm [ESI][EDI][EAX][EBX][EDX] modify [ECX]
void put_picture2picture(const word *source,word *target,int xp,int yp);
//#pragma aux put_picture2picture parm [ESI][EDI][EAX][EDX] modify [ECX]

void draw_rounded_rectangle(int x, int y, int xs, int ys, int radius,
                            int stroke_color, int fill_color);
void greyscale_rectangle_ex(int x, int y, int xs, int ys, uint16_t *screen_address, size_t line_width);
void greyscale_rectangle(int x, int y, int xs, int ys);

#define swap_int(a,b) do  {int c=a;a=b;b=c;} while (0);
