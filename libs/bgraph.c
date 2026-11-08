#include "types.h"
//#include <vesa.h>
#include <dpmi.h>
#include <i86.h>

#include <stdio.h>
#include <stdlib.h>
#include <graph.h>
#include "bgraph.h"

word *lbuffer;
word *screen;
word curcolor,charcolors[7] = {0x0000,0x03E0,0x0380,0x0300,0x0280,0x0000,0x0000};
longint linelen;
const word *curfont;
word *writepos,writeposx;
byte fontdsize=0;
byte *palmem,*xlatmem;
void (*showview)(word,word,word,word);
char line480=0;
int32_t screen_buffer_size=512000;

const void *mscursor,*mssavebuffer=NULL;
integer mscuroldx=0,mscuroldy=0;
integer msshowx=0,msshowy=0;
int32_t pictlen; // Tato promenna je pouze pouzita v BGRAPH1.ASM

void line32(word x1,word y1, word x2, word y2)
  {
  line_32(x1,y1,(x2-x1),(y2-y1));
  }

void position(word x,word y)
  {
  writeposx=x;
  writepos=getadr32(x,y);
  }

void outtext_ex(const char *text, int space)
  {
  int pos;

  if (fontdsize)
     while (*text)
     {
     char2_32(writepos,curfont,*text);
     pos=((charsize(curfont,*text) & 0xff)<<1)+space
     writepos+=pos
     writeposx+=pos;text++;
     }
  else
   while (*text)
     {
     char_32(writepos,curfont,*text);
     pos=(charsize(curfont,*text) & 0xff)+space;
     writepos+=pos;
     writeposx+=pos;text++;
     }
  }
void outtext(const char *text) {
    outtext_ex(text,0);

}




int initmode32()
  {
  MODEinfo data;

  getmodeinfo(&data,0x11e-line480*0xe);
  if (!(data.modeattr & MA_SUPP)) return -1;
  if (!(data.modeattr & MA_LINEARFBUF)) return -2;
  setvesamode(0x411e-line480*0xe,-1);
  lbuffer=(word *)physicalalloc((int32_t)data.linearbuffer,screen_buffer_size);
  screen=lbuffer;
  linelen=640*2;
  showview=showview32;
  screen=(void *)malloc(screen_buffer_size);
  return 0;
  }
int initmode256(void *paletefile)
  {
  MODEinfo data;

  getmodeinfo(&data,0x100+line480);
  if (!(data.modeattr & MA_SUPP)) return -1;
  if (!(data.modeattr & MA_LINEARFBUF)) return -2;
  setvesamode(0x4100+line480,-1);
  lbuffer=(word *)physicalalloc((int32_t)data.linearbuffer,screen_buffer_size>>1);
  screen=lbuffer;
  linelen=640*2;
  palmem=(char *)paletefile;
  xlatmem=palmem+768;
  setpal((void *)palmem);
  showview=showview256;
  screen=(void *)malloc(screen_buffer_size);
  return 0;
  }


int initmode_lo(void *paletefile)
  {
  _setvideomode(_MRES256COLOR);
  palmem=(char *)paletefile;
  xlatmem=palmem+768;
  setpal((void *)palmem);
  linelen=640*2;
  lbuffer=(void *)0xa0000;
  showview=showview_lo;
  screen=(void *)malloc(512000);
  return 0;
  }

void closemode()
  {
  free(screen);
  _setvideomode(0x3);
  }

void showview32(word x,word y,word xs,word ys)
  {
  register longint a;

  if (x>640 || y>400) return;
  if (xs==0) xs=640;
  if (ys==0) ys=400;
  if (x+xs>640) xs=640-x;
  if (y+ys>400) ys=400-y;
  if (xs>500 && ys>320)
     {
     redraw32(screen,lbuffer,NULL);
     return;
     }
  a=(x<<1)+linelen*y;
  redrawbox32(xs,ys,(void *)((char *)screen+a),(void *)((char *)lbuffer+a));
  }

void showview256(word x,word y,word xs,word ys)
  {
  register longint a;

  if (xs==0) xs=640;
  if (ys==0) ys=400;
  x&=0xfffe;y&=0xfffe;xs+=2;ys+=2;
  if (x>640 || y>400) return;
  if (x+xs>640) xs=640-x;
  if (y+ys>400) ys=400-y;
  if (xs>500 && ys>320)
     {
     redraw256(screen,lbuffer,xlatmem);
     return;
     }
  a=(x<<1)+linelen*y;
  redrawbox256(xs,ys,(void *)((char *)screen+a),(void *)((char *)lbuffer+(a>>1)),xlatmem);
  }

void showview_lo(word x,word y,word xs,word ys)
  {
  register longint a,b;

  if (xs==0) xs=640;
  if (ys==0) ys=400;
  x&=0xfffe;y&=0xfffe;xs+=2;ys+=2;
  if (ys==0) ys=400;
  if (x+xs>640) xs=640-x;
  if (y+ys>400) ys=400-y;
  if (xs>500 && ys>320)
     {
     redraw_lo(screen,lbuffer,xlatmem);
     return;
     }
  a=(x<<1)+linelen*y;
  b=(x>>1)+320*(y>>1);
  redrawbox_lo(xs,ys,(void *)((char *)screen+a),(void *)((char *)lbuffer+b),xlatmem);
  }




void show_ms_cursor(char copy,integer x, integer y)
  {
  integer xs,ys;

  xs=*(integer *)mscursor;
  ys=*((integer *)mscursor+1);
  get_picture(x,y,xs,ys,mssavebuffer);
  put_picture(x,y,mscursor);
  if (copy)
     {
    mscuroldx=x;
    mscuroldy=y;
     }
  }

void hide_ms_cursor()
  {
  put_picture(mscuroldx,mscuroldy,mssavebuffer);
  }

void *register_ms_cursor(const void *cursor)
  {
  integer xs,ys;

  mscursor=cursor;
  xs=*(integer *)mscursor;
  ys=*((integer *)mscursor+1);
  if (mssavebuffer!=NULL) free(mssavebuffer);
  mssavebuffer=malloc(xs*ys*2+10);//5 bajtu pro strejcka prihodu
  return mssavebuffer;
  }

void move_ms_cursor(integer newx,integer newy)
  {
  integer xs,ys;

  if (newx<0) newx=0;
  if (newy<0) newy=0;
  if (newx>639) newx=639;
  if (newy>399) newy=399;
  xs=*(integer *)mscursor;
  ys=*((integer *)mscursor+1);
  put_picture(mscuroldx,mscuroldy,mssavebuffer);
  show_ms_cursor(0,newx,newy);
  showview(msshowx,msshowy,xs,ys);
  mscuroldx=newx;mscuroldy=newy;
  msshowx=newx;msshowy=newy;
  showview(msshowx,msshowy,xs,ys);
  }

int text_height(char *text)
  {
  char max=0,cur;

  while (*text)
     if ((cur=charsize(curfont,*text++)>>8)>max) max=cur;
  return max<<fontdsize;
  }

int text_width(char *text)
  {
  int suma=0;

  while (*text)
     suma+=charsize(curfont,*text++) & 0xff;
  return suma<<fontdsize;
  }


void set_aligned_position(int x,int y,char alignx,char aligny,char *text)
  {
  switch (alignx)
     {
     case 1:x-=text_width(text)>>1;break;
     case 2:x-=text_width(text);break;
     }
  switch (aligny)
     {
     case 1:y-=text_height(text)>>1;break;
     case 2:y-=text_height(text);break;
     }
  position(x,y);
  }


