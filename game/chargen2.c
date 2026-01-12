//CHARACTER GENERATOR

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include <libs/types.h>
#include <libs/event.h>
#include <libs/memman.h>
#include <libs/devices.h>
#include <libs/bmouse.h>
#include <libs/bgraph.h>
#include <platform/sound.h>
#include <libs/gui.h>
#include <libs/basicobj.h>
#include "engine1.h"
#include <libs/pcx.h>
#include "globals.h"
#include <stddef.h>

//there is defined procedures from source "INV.C"

void display_items_wearing(THUMAN *h);


#define MAX_XICHTS 8
#define MAX_CHARS 3

#define INV_DESK 266
#define INV_DESC_X 285
#define INV_DESC_Y (SCREEN_OFFLINE+20)
#define HUMAN_X 40
#define HUMAN_Y 328


#define X_SKIP_X 64
#define X_SKIP_Y 85

typedef struct staty
  {
  char zivl;
  char zivh;
  char manl;
  char manh;
  char konl;
  char konh;
  char akc;
  }T_STATY;

#define MAX_RACES 5
static char women[MAX_XICHTS]={0,1,0,0,0,1,0,0};//,0,0};
static int cur_edited=0;
static int cur_xicht=-1;
int cur_page=1;
static int loc_select=-1;

#define CLK_PAGE1 3



static T_CLK_MAP clk_page1[]=
  {

  };

static void display_character(THUMAN *p)
  {
  word *w;
 put_picture(4,SCREEN_OFFLINE,ablock(H_IOBLOUK));
 if (p->used)
     {
     w=ablock(hl_ptr+MAX_XICHTS+(p->xicht));
     put_picture(HUMAN_X+30,HUMAN_Y-w[1],w);
     }
  }
/*
static void display_all_xichts()
  {
  int i,a,x,y;

  a=0;
  x=INV_DESC_X;y=INV_DESC_Y;
  for(i=0;i<MAX_XICHTS;i++)
     {
     put_image(ablock(hl_ptr+i),x+640*y+screen,0,54,75);
     if (i==cur_xicht) rectangle(x,y,x+53,y+74,0x7fff);
     x+=X_SKIP_X;
     a++;if (a>4)
        {
        a=0;
        x=INV_DESC_X;
        y+=X_SKIP_Y;
        }
     }
  }
*/
/*
static void display_race_line(int line,char pushed)
  {
  int x,y;
  char *c;

  set_font(H_FBOLD,pushed?0x3e0:0x7fff);
  x=INV_DESK+137;y=INV_DESC_Y+170+22*line;
  put_textured_bar(ablock(H_BUTBIG),x,y,100,20,0,20*pushed);
  x+=49+pushed;y+=9+pushed;
  if (cur_xicht==-1 || !women[cur_xicht]) c=texty[line+110];else c=texty[line+115];
  set_aligned_position(x,y,1,1,c);outtext(c);
  }
*/

static void tlac2(int x,int y,char *text,int pushed)
  {
  set_font(H_FBOLD,pushed?0x3e0:0x7fff);
  put_textured_bar(ablock(H_BUTSMALL),x,y,56,20,0,20*pushed);
  x+=27+pushed;y+=9+pushed;
  set_aligned_position(x,y,1,1,text);outtext(text);
  }

static void tlac2_press(int x,int y,char *text)
  {
  schovej_mysku();
  tlac2(x,y,text,1);
  ukaz_mysku();
  showview(x,y,56,20);
  }

/*
static void display_page2()
  {
  int i,x,y;
  char s[10];

  set_font(H_FBOLD,0x7fff);
  position(INV_DESC_X,INV_DESC_Y);outtext(texty[122]);
  x=INV_DESC_X+10;y=INV_DESC_Y+100;
  for(i=0;i<4;i++)
     {
     position(x,y);outtext(texty[i+10]);
     sprintf(s,"%d",postavy[cur_edited].stare_vls[i+VLS_SILA]);
     set_aligned_position(x+140,y,2,0,s);outtext(s);
     tlac2(x+150,y-5,"-",0);
     tlac2(x+210,y-5,"+",0);
     y+=30;
     }
  position(x,y);outtext(texty[19]);
  sprintf(s,"%d",postavy[cur_edited].bonus);
  set_aligned_position(x+140,y,2,0,s);outtext(s);
  }
*/
static void zobraz_staty(T_STATY *st)
  {
  char s[100];

  set_font(H_FONT6,(28*1024+16*32+2));
  sprintf(s,texty[2],st->zivl,st->zivh);
  position(250,3);outtext(s);
  sprintf(s,texty[3],st->manl,st->manh);
  position(350,3);outtext(s);
  sprintf(s,texty[4],st->konl,st->konh);
  position(450,3);outtext(s);
  sprintf(s,texty[5],st->akc);
  position(550,3);outtext(s);
  }

void redraw_generator()
  {
  T_STATY z;
  memset(&z,20,sizeof(z));
  schovej_mysku();
  put_picture(INV_DESK,SCREEN_OFFLINE,ablock(H_GENERACE));
  put_picture(0,SCREEN_OFFLINE,ablock(H_IOBLOUK));
  bott_draw(1);
  memcpy(screen+(480-102)*640,ablock(H_BOTTBAR),640*102*2);
  put_picture(0,0,ablock(H_GEN_TOPBAR));
  zobraz_staty(&z);
  ukaz_mysku();
  showview(0,0,0,0);
  }

static char page1_xicht(int id,int xa,int ya,int xr,int yr)
  {
  THUMAN *p;
  char s[15];
  xa,ya,id;
  if (xr%X_SKIP_X>54 || yr%X_SKIP_Y>75) return 0;
  xr/=X_SKIP_X;
  yr/=X_SKIP_Y;
  cur_xicht=yr*5+xr;
  sprintf(s,CHAR_NAME,cur_xicht);
  def_handle(H_CHARS+cur_edited,s,pcx_fade_decomp,SR_BGRAFIKA);
  sprintf(s,XICHT_NAME,cur_xicht);
  def_handle(H_XICHTY+cur_edited,s,pcx_8bit_decomp,SR_BGRAFIKA);
  p=postavy+cur_edited;
  p->used=1;
  p->lives=1;
  p->xicht=cur_xicht;
  p->sektor=viewsector;
  prepocitat_postavu(p);
  redraw_generator();
  return 1;
  }

static char page1_tlac1(int id,int xa,int ya,int xr,int yr)
  {
  yr/=22;yr,xa,ya,id,xr;
  if (yr<MAX_RACES)loc_select=yr;
  redraw_generator();
  return 1;
  }

static void timered_redraw_call(THE_TIMER *t)
  {
  t;
  redraw_generator();
  }

static void timered_redraw()
  {
  add_to_timer(-1,5,1,timered_redraw_call);
  }

static void generate_vlastnosti(THUMAN *p, int povolani)
  {
  short *w=&p->vlastnosti;
  //common definiton
  p->used=1;
  p->groupnum=1;
  p->sipy=0;
  p->inv_size=6;
  p->level=1;
  p->exp=rnd(200);
  p->female=women[p->xicht];
  switch(povolani)
     {
     case 0:w[VLS_SILA]=5+rnd(5);
            w[VLS_SMAGIE]=0;
            w[VLS_POHYB]=rnd(7)+2;
            w[VLS_OBRAT]=rnd(5)+1;
            w[VLS_MAXHIT]=10+rnd(10);
            w[VLS_MAXMANA]=0;
            w[VLS_KONDIC]=rnd(10)+10;
            w[VLS_HPREG]=2;
            w[VLS_MPREG]=1;
            w[VLS_VPREG]=1;
            p->bonus_zbrani[TPW_MEC]=1;
            p->bonus_zbrani[TPW_SEKERA]=1;
            break;
     case 1:w[VLS_SILA]=rnd(5)+3;
            w[VLS_SMAGIE]=rnd(5);
            w[VLS_POHYB]=rnd(7)+2;
            w[VLS_OBRAT]=rnd(7)+1;
            w[VLS_MAXHIT]=10+rnd(10);
            w[VLS_MAXMANA]=rnd(10);
            w[VLS_KONDIC]=rnd(10)+10;
            w[VLS_HPREG]=1;
            w[VLS_MPREG]=1;
            w[VLS_VPREG]=2;
            p->bonus_zbrani[TPW_MEC]=1;
            p->bonus_zbrani[TPW_DYKA]=1;
            break;
     case 2:w[VLS_SILA]=rnd(4)+1;
            w[VLS_SMAGIE]=rnd(5)+5;
            w[VLS_POHYB]=rnd(4)+1;
            w[VLS_OBRAT]=rnd(4)+1;
            w[VLS_MAXHIT]=5+rnd(5);
            w[VLS_MAXMANA]=5+rnd(10);
            w[VLS_KONDIC]=rnd(5)+10;
            w[VLS_HPREG]=1;
            w[VLS_MPREG]=2;
            w[VLS_VPREG]=1;
            p->bonus_zbrani[TPW_HUL]=1;
            p->bonus_zbrani[TPW_DYKA]=1;
            break;
     case 3:w[VLS_SILA]=rnd(4)+1;
            w[VLS_SMAGIE]=0;
            w[VLS_POHYB]=rnd(5)+4;
            w[VLS_OBRAT]=rnd(5)+5;
            w[VLS_MAXHIT]=10+rnd(5);
            w[VLS_MAXMANA]=0;
            w[VLS_KONDIC]=rnd(5)+10;
            w[VLS_HPREG]=1;
            w[VLS_MPREG]=1;
            w[VLS_VPREG]=1;
            p->bonus_zbrani[TPW_STRELNA]=3;
            break;
     case 4:w[VLS_SILA]=rnd(4)+1;
            w[VLS_SMAGIE]=0;
            w[VLS_POHYB]=rnd(7)+2;
            w[VLS_OBRAT]=rnd(4)+6;
            w[VLS_MAXHIT]=5+rnd(5);
            w[VLS_MAXMANA]=5+rnd(10);
            w[VLS_KONDIC]=rnd(5)+10;
            w[VLS_HPREG]=1;
            w[VLS_MPREG]=1;
            w[VLS_VPREG]=1;
            p->bonus_zbrani[TPW_DYKA]=3;
            break;
     }
  p->lives=w[VLS_MAXHIT];
  p->mana=w[VLS_MAXMANA];
  p->kondice=w[VLS_KONDIC];
  p->jidlo=MAX_HLAD(p);
  p->voda=MAX_ZIZEN(p);
  memcpy(p->stare_vls,p->vlastnosti,sizeof(p->vlastnosti));
  }

static char page1_tlac2(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya,xr,yr;
  tlac2_press(INV_DESK+290,340,texty[120]);
  timered_redraw();
  if (cur_xicht==-1 || loc_select==-1) return 1;
  cur_page=2;
  generate_vlastnosti(postavy+cur_edited,loc_select);
  return 1;
  }


char enter_generator()
  {
  int i;

  curcolor=0;bar(0,0,639,479);
  for(i=0;i<MAX_XICHTS;i++)
     {
     char s[15];

     sprintf(s,CHAR_NAME,i);
     def_handle(hl_ptr+i+MAX_XICHTS,s,pcx_8bit_decomp,SR_BGRAFIKA);
     }
  for(i=0;i<MAX_XICHTS;i++)
     {
     char s[15];

     sprintf(s,XICHT_NAME,i);
     def_handle(hl_ptr+2*MAX_XICHTS+i,s,pcx_8bit_decomp,SR_BGRAFIKA);
     }
  memset(postavy,0,sizeof(postavy));
  redraw_generator();
  change_click_map(clk_page1,CLK_PAGE1);
  }
