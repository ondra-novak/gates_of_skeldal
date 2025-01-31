#include <platform/platform.h>
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "bgraph.h"
#include "memman.h"

word *screen;
word curcolor,charcolors[7] = {0x0000,RGB555(0,31,0),RGB555(0,28,0),RGB555(0,24,0),RGB555(0,20,0),0x0000,0x0000};
word *curfont,*writepos,writeposx;
byte fontdsize=0;
void (*showview)(word,word,word,word);
char line480=0;
int32_t screen_buffer_size=0;
char screenstate=0;
char __skip_change_line_test=0;
char no_restore_mode=0;

void *mscursor,*mssavebuffer=NULL;
integer mscuroldx=0,mscuroldy=0;
integer mscuroldxs=1,mscuroldys=1;
char write_window=0;
int32_t pictlen; // Tato promenna je pouze pouzita v BGRAPH1.ASM

void text_mode();

void wait_retrace();




void line32(word x1,word y1, word x2, word y2)
  {
  line_32(x1,y1,(x2-x1),(y2-y1));
  }

void position(word x,word y)
  {
  writeposx=x;
  writepos=getadr32(x,y);
  }

void rel_position_x(word x)
  {
  writeposx+=x;
  writepos+=x;
  }


void outtext(const char *text)
  {
  byte pos;

  if (fontdsize)
     while (*text)
     {
     char2_32(writepos,curfont,*text);
     pos=(charsize(curfont,*text) & 0xff)<<1;
     writepos+=pos;
     writeposx+=pos;text++;
     }
  else
   while (*text)
     {
     char_32(writepos,curfont,*text);
     pos=charsize(curfont,*text) & 0xff;
     writepos+=pos;
     writeposx+=pos;text++;
     }
  }

/*MODEinfo vesadata[3];
SVGAinfo svgadata[3];
int lastbank=0;
int granuality=0;
int gran_mask=0;
word gr_page_end=0;
int gr_end_screen=0;

word *mapvesaadr(word *a);
#pragma aux mapvesaadr parm [edi] value [edi]

void write_vesa_info(int mode)
  {
  char c[20];

  getsvgainfo(&svgadata);
  printf("VIDEO mem   %5dKb\n"
         "Oem:        %s\n\n",
         svgadata[0].memory*64,
         svgadata[0].oemstr);
  getmodeinfo(&vesadata,mode);
  if (vesadata[0].modeattr & MA_SUPP)
     {
     if (vesadata[0].modeattr & MA_LINEARFBUF) sprintf(c,"%8Xh",(int32_t)vesadata[0].linearbuffer); else strcpy(c,"None");
    printf("Mode:        %04X \n"
           "WinA           %02X\n"
           "WinB           %02X\n"
           "Granuality: %5dKB\n"
           "WinSize:    %5dKB\n"
           "Xres:       %5d\n"
           "Yres:       %5d\n"
           "Bppix:      %5d\n"
           "Lbuffer:    %s\n\n",
           mode,
           vesadata[0].winaattr,
           vesadata[0].winbattr,
           vesadata[0].wingran,
           vesadata[0].winsize,
           vesadata[0].xres,
           vesadata[0].yres,
           vesadata[0].bppix,
           c);
    }
  else printf("Mode %04X not currently supported!!!\n\n");
//  printf("--- Hit ENTER if values are correct or press CTRL+Break ---\n");
//  getche();
  delay(300);
  }
*/
void showview_dx(word x,word y,word xs,word ys);
//void showview64b(word x,word y,word xs,word ys);
/*void showview32b(word x,word y,word xs,word ys)
  {
  register longint a,b;

  if (x>640 || y>480) return;
  if (xs==0) xs=640;
  if (ys==0) ys=480;
  xs++;ys++;
  x&=~3;
  xs=(xs & ~3)+4;
  if (x+xs>640) xs=640-x;
  if (y+ys>480) ys=480-y;
  if (xs>550 && ys>400)
     {
     redraw32bb(screen,NULL,NULL);
     return;
     }
  a=(x<<1)+linelen*y;
  b=y*1280+x*2;
  redrawbox32bb(xs,ys,(void *)((char *)screen+a),(void *)b);
  }
*/
/*
void set_scan_line(int newline);
#pragma aux set_scan_line=\
           "mov  eax,4f06h"\
           "xor  ebx,ebx"\
           "int 10h"\
   parm [ecx] modify [eax ebx];

int get_scan_line();
#pragma aux get_scan_line=\
           "mov  eax,4f06h"\
           "mov  bh,01"\
           "int  10h"\
   modify[eax ebx] value[ecx];

void *create_hixlat()
  {
  word *s;
  word i;

  s=NewArr(word,32768);
  for (i=0;i<32768;i++) s[i]=((i & ~0x1f)<<1) | (i & 0x1f);
  return (byte *)s;
  }

int initmode64b(void *paletefile)
  {
  int i;

  getmodeinfo(&vesadata,0x111);
  if (!(vesadata[0].modeattr & MA_SUPP)) return -1;
  //write_vesa_info(0x110);
  if (vesadata[0].winaattr & (WA_SUPP | WA_WRITE)) write_window=0;
  else if (vesadata[0].winbattr & (WA_SUPP | WA_WRITE)) write_window=1;
  else return -1;
  i=vesadata[0].wingran*1024;
  granuality=0;lastbank=0;
  while (i>>=1) granuality++;
  gran_mask=(1<<granuality)-1;
  gr_end_screen=0xa0000+gran_mask+1;
  gr_page_end=gran_mask+1;
  setvesamode(0x111,-1);
  lbuffer=(word *)0xa0000;
  screen=lbuffer;
  linelen=640*2;
  showview=showview64b;
  screen=(void *)malloc(screen_buffer_size);
  banking=1;
  screenstate=1;
  xlatmem=paletefile;
  return 0;

  }

int initmode32b()
  {
  int i;
  getmodeinfo(&vesadata,0x110);
  if (!(vesadata[0].modeattr & MA_SUPP)) return -1;
  //write_vesa_info(0x110);
  if (vesadata[0].winaattr & (WA_SUPP | WA_WRITE)) write_window=0;
  else if (vesadata[0].winbattr & (WA_SUPP | WA_WRITE)) write_window=1;
  else return -1;
  i=vesadata[0].wingran*1024;
  granuality=0;lastbank=0;
  while (i>>=1) granuality++;
  gran_mask=(1<<granuality)-1;
  gr_end_screen=0xa0000+gran_mask+1;
  gr_page_end=gran_mask+1;
  setvesamode(0x110,-1);
  lbuffer=(word *)0xa0000;
  screen=lbuffer;
  linelen=640*2;
  showview=showview32b;
  screen=(void *)malloc(screen_buffer_size);
  banking=1;
  screenstate=1;
  return 0;
  }



int initmode32bb()
  {
  int i;
  getmodeinfo(&vesadata,0x110);
  if (!(vesadata[0].modeattr & MA_SUPP)) return -1;
  //write_vesa_info(0x110);
  if (vesadata[0].winaattr & (WA_SUPP | WA_WRITE)) write_window=0;
  else if (vesadata[0].winbattr & (WA_SUPP | WA_WRITE)) write_window=1;
  else return -1;
  i=vesadata[0].wingran*1024;
  granuality=0;lastbank=0;
  while (i>>=1) granuality++;
  setvesamode(0x110,-1);
  set_scan_line(1024);
  if (get_scan_line()!=1024 && !__skip_change_line_test)
     {
     text_mode();
     return -10;
     }
  lbuffer=(word *)0xa0000;
  screen=lbuffer;
  linelen=640*2;
  showview=showview32b;
  screen=(void *)malloc(screen_buffer_size);
  banking=1;
  screenstate=1;
  return 0;
  }



word *mapvesaadr1(word *a)
  {
  word bank;

  bank=(int32_t)a>>16;
  if (bank!=lastbank)
     {
     lastbank=bank;
     bank=bank;
          {
           union REGS regs;
           regs.w.ax = 0x4f05;
           regs.w.bx = write_window;
           regs.w.dx = bank;
           int386 (0x10,&regs,&regs); // window A
          }
     }
 return (word *)(((int32_t)a & 0xffff)+0xa0000);
}

void switchvesabank(word bank)
#pragma aux switchvesabank parm [eax]
  {
           union REGS regs;
           regs.w.ax = 0x4f05;
           regs.w.bx = 0;
           regs.w.dx = bank;
           int386 (0x10,&regs,&regs); // window A
  }

*/
int initmode(const INI_CONFIG_SECTION *display_config, const char *app_name)
  {
    if (!game_display_init(display_config, app_name)) return -1;
  showview=game_display_update_rect;
  screenstate=1;
  return 0;
  }


  /*
int initmode256(void *paletefile)
  {
  MODEinfo data;

  getmodeinfo(&data,0x100+line480);
  if (!(data.modeattr & MA_SUPP)) return initmode256b(paletefile);
  if (!(data.modeattr & MA_LINEARFBUF)) return initmode256b(paletefile);
  //write_vesa_info(0x101);
  setvesamode(0x4101,-1);
  if (lbuffer==NULL)lbuffer=(word *)physicalalloc((int32_t)data.linearbuffer,screen_buffer_size>>1);
  screen=lbuffer;
  linelen=640*2;
  palmem=(char *)paletefile;
  xlatmem=palmem+768;
  setpal((void *)palmem);
  showview=showview256;
  screen=(void *)malloc(screen_buffer_size);
  screenstate=1;
  banking=0;
  return 0;
  }


void showview256b(word x,word y,word xs,word ys)
  {
  register longint a,b;

  if (x>640 || y>480) return;
  if (xs==0) xs=640;
  if (ys==0) ys=480;
  xs++;ys++;
  x&=~3;
  xs=(xs & ~3)+4;
  y&=~1;
  ys=(ys & ~1)+2;
  if (x+xs>640) xs=640-x;
  if (y+ys>480) ys=480-y;
  if (xs>550 && ys>400)
     {
     redraw256b(screen,0,xlatmem);
     return;
     }
  a=(x<<1)+linelen*y;
  b=y*640+x;
  redrawbox256b(xs,ys,(void *)((char *)screen+a),(void *)b,xlatmem);
  }


int initmode256b(void *paletefile)
  {
  int i;
  getmodeinfo(&vesadata,0x100);
  if (!(vesadata[0].modeattr & MA_SUPP)) return -1;
  //write_vesa_info(0x101);
  i=vesadata[0].wingran*1024;
  if (vesadata[0].winaattr & (WA_SUPP | WA_WRITE)) write_window=0;
  else if (vesadata[0].winbattr & (WA_SUPP | WA_WRITE)) write_window=1;
  else return -1;
  granuality=0;lastbank=0;
  while (i>>=1) granuality++;
  gran_mask=(1<<granuality)-1;
  gr_end_screen=0xa0000+gran_mask+1;
  gr_page_end=gran_mask+1;
  setvesamode(0x101,-1);
  lbuffer=(word *)0xa0000;
  screen=lbuffer;
  palmem=(char *)paletefile;
  xlatmem=palmem+768;
  setpal((void *)palmem);
  linelen=640*2;
  showview=showview256b;
  screen=(void *)malloc(screen_buffer_size);
  banking=1;
  screenstate=1;
  return 0;
  }


void init_lo();
#pragma aux init_lo modify[eax ebx ecx edx esi edi]

int initmode_lo(void *paletefile)
  {
  init_lo();
  palmem=(char *)paletefile;
  xlatmem=palmem+768;
  setpal((void *)palmem);
  linelen=640*2;
  lbuffer=0;
  showview=showview_lo;
  screen=(void *)malloc(screen_buffer_size);
  screenstate=1;
  banking=1;
  return 0;
  }
*/

void closemode()
  {
  if (screenstate)
     {
     game_display_close();
     }
  screenstate=0;

  }

/*
static void showview64b(word x,word y,word xs,word ys)
  {
  register longint a;

  if (x>640 || y>480) return;
  if (xs==0) xs=640;
  if (ys==0) ys=480;
  xs+=2;ys+=2;
  if (x+xs>640) xs=640-x;
  if (y+ys>480) ys=480-y;
  if (xs>550 && ys>400)
     {
     redraw64b(screen,NULL,xlatmem);
     return;
     }
  a=(x<<1)+linelen*y;
  redrawbox64b(xs,ys,(void *)((char *)screen+a),(void *)((char *)a),xlatmem);
  }


void showview256(word x,word y,word xs,word ys)
  {
  register longint a;

  if (xs==0) xs=640;
  if (ys==0) ys=480;
  x&=0xfffe;y&=0xfffe;xs+=2;ys+=2;
  if (x>640 || y>480) return;
  if (x+xs>640) xs=640-x;
  if (y+ys>480) ys=480-y;
  if (xs>550 && ys>400)
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
  if (ys==0) ys=480;
  if (ys==0) ys=480;
  x&=0xfffe;y&=0xfffe;xs+=2;ys+=2;
  if (x+xs>640) xs=640-x;
  if (y+ys>480) ys=480-y;
  if (xs>550 && ys>400)
     {
     redraw_lo(screen,lbuffer,xlatmem);
     return;
     }
  a=(x<<1)+linelen*y;
  b=x+640*y;
  redrawbox_lo(xs,ys,(void *)((char *)screen+a),(void *)((char *)lbuffer+b),xlatmem);
  }



*/
void show_ms_cursor(integer x,integer y)
  {
  integer xs,ys;

  int mx =  DxGetResX() - 1;
  int my =  DxGetResY() - 1;

  if (x<0) x=0;
  if (x>mx) mx=639;
  if (y<0) y=0;
  if (y>my) my=479;
  xs=*(integer *)mscursor;
  ys=*((integer *)mscursor+1);
  get_picture(x,y,xs,ys,mssavebuffer);
  put_picture(x,y,mscursor);
  mscuroldx=x;
  mscuroldy=y;
  }

void hide_ms_cursor()
  {
  put_picture(mscuroldx,mscuroldy,mssavebuffer);

  }

void redraw_ms_cursor_on_screen(void) {
    if (mssavebuffer) {
      integer xs=*(integer *)mssavebuffer;
      integer ys=*((integer *)mssavebuffer+1);
      showview(mscuroldx,mscuroldy,xs,ys);
    }
}


void *register_ms_cursor(void *cursor)
  {
  integer xs,ys;

  mscursor=cursor;
  xs=*(integer *)mscursor;
  ys=*((integer *)mscursor+1);
  if (mssavebuffer!=NULL) free(mssavebuffer);
  mssavebuffer=malloc(xs*ys*2+10);//5 bajtu pro strejcka prihodu
  return mssavebuffer;
  }

void move_ms_cursor(integer newx,integer newy,char nodraw)
  {
  integer xs,ys;
  int mx =  DxGetResX() - 1;
  int my =  DxGetResY() - 1;
  static integer msshowx=0,msshowy=0;

  xs=*(integer *)mscursor;
  ys=*((integer *)mscursor+1);
  if (nodraw)
     {
     showview(msshowx,msshowy,xs,ys);
     msshowx=mscuroldx;
     msshowy=mscuroldy;
     return;
     }
  if (newx<0) newx=0;
  if (newy<0) newy=0;
  if (newx>mx) newx=mx;
  if (newy>my) newy=my;
  put_picture(mscuroldx,mscuroldy,mssavebuffer);
  show_ms_cursor(newx,newy);
  mscuroldx=newx;mscuroldy=newy;
  showview(msshowx,msshowy,mscuroldxs,mscuroldys);
  if (mscuroldx!=msshowx || mscuroldy!=msshowy)showview(mscuroldx,mscuroldy,mscuroldxs,mscuroldys);
  msshowx=newx;msshowy=newy;
  showview(msshowx,msshowy,xs,ys);
  mscuroldxs=xs;
  mscuroldys=ys;
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

void rectangle(int x1,int y1,int x2,int y2,int color)
  {
  curcolor=color;
  hor_line32(x1,y1,x2);
  hor_line32(x1,y2,x2);
  ver_line32(x1,y1,y2);
  ver_line32(x2,y1,y2);
  }

