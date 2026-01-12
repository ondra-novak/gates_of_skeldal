//CHARACTER GENERATOR
#include <platform/platform.h>
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
void inv_display_vlastnosti();;
extern void (*inv_redraw)(char);
void write_human_big_name(char *c);

#define MSG_COLOR1 RGB555(30,30,23)


#define MAX_XICHTS 8
#define MAX_CHARS 6

#define XICHT_STEP 40

#define INV_DESK 266
#define INV_DESK_Y (SCREEN_OFFLINE)
#define INV_DESC_X 285
#define INV_DESC_Y (SCREEN_OFFLINE+20)
#define HUMAN_X 40
#define HUMAN_Y 328


#define X_SKIP_X 64
#define X_SKIP_Y 85

typedef struct staty
  {
  int zivl;
  int zivh;
  int manl;
  int manh;
  int konl;
  int konh;
  int akc;
  }T_STATY;

static T_STATY cur_stats;

word color_butt_on[7]={0,RGB555(31,27,4),RGB555(30,26,4),RGB555(29,25,4)};
word color_butt_off[7]={0,RGB555(10,10,10),RGB555(10,10,10),RGB555(10,10,10)};


typedef struct vlasts
  {
  int sill;
  int silh;
  int smgl;
  int smgh;
  int pohl;
  int pohh;
  int obrl;
  int obrh;
  int hpreg;
  int mpreg;
  int vpreg;
  }T_VLASTS;

static T_VLASTS cur_vls;
static T_VLASTS rohy[9]=
  {
  {17,22, 5,10,17,22, 9,14, 3, 2, 3},
  {13,18,10,15,20,25, 5,10, 3, 3, 2},
  { 9,14,15,20,17,22, 9,14, 3, 3, 2},
  { 5,10,20,25,13,18,13,18, 2, 4, 2},
  { 9,14,15,20, 9,14,17,22, 2, 3, 2},
  {13,18,10,15, 5,10,20,25, 2, 2, 4},
  {17,22, 5,10, 9,14,17,22, 3, 1, 2},
  {20,25, 0, 5,13,18,13,18, 4, 1, 2},
  {12,17,12,17,12,17,12,17, 2, 2, 2},
  };

#define CALC_DIFF(x,y,angl) ((x)+(((((y)-(x))<<4)*((angl)<<4)/720+8)>>4))
#define CALC_DIFF2(x,y,pol) ((x)+(((((y)-(x))<<4)*((pol)<<4)/(PERLA_MAXPOLOMER<<4)+8)>>4))
#define DIFF_RAND(x,y) ((x)+rnd((y)-(x)+1))

#define MAX_RACES 5
static uint8_t women[MAX_XICHTS]={0,0,0,0,1,1,1,1};
static uint8_t poradi[MAX_XICHTS]={0,2,3,4,1,5,6,7};
static uint8_t disable[MAX_XICHTS];
static int cur_edited=0;
static int cur_angle=90;
static int cur_polomer=0;
static int cur_xicht=-1;
static char shut_downing_text=0;
static int save_values[POCET_POSTAV][2];
static char del_mode=0;
static char was_enter=0;


#define PERLA_STRED_X (INV_DESK+189)
#define PERLA_STRED_Y (INV_DESK_Y+217)


#define PERLA_POLOMER cur_polomer
#define PERLA_MAXPOLOMER 75
short *pod_perlou=NULL;
short pod_perlou_x=0;
short pod_perlou_y=0;
char *error_text=NULL;



static char select_xicht(int id,int xa,int ya,int xr,int yr);
static char vol_vlastnosti(int id,int xa,int ya,int xr,int yr);
static char go_next_page(int id,int xa,int ya,int xr,int yr);
char vls_click(int id,int xa,int ya,int xr,int yr);
static char view_another_click2(int id,int xa,int ya,int xr,int yr);
//char edit_another_click(int id,int xa,int ya,int xr,int yr);
//static char edit_another_click2(int id,int xa,int ya,int xr,int yr);
static char gen_exit_editor(int id,int xa,int ya,int xr,int yr);

static void zobraz_staty(T_VLASTS *st);

#define CLK_PAGE1 6


static T_CLK_MAP clk_page1[]=
  {
  {0,28+INV_DESK,18+INV_DESK_Y,334+INV_DESK,55+INV_DESK_Y,select_xicht,2,-1},
  {0,78+INV_DESK,97+INV_DESK_Y,314+INV_DESK,340+INV_DESK_Y,vol_vlastnosti,2+1,-1},
  {-1,520,378,639,479,go_next_page,2,-1},
  //{-1,54,378,497,479,edit_another_click,2+8,-1},
  {2,0,0,639,479,gen_exit_editor,8,-1},
  {0,0,0,639,479,empty_clk,0xff,-1},
  };

#define CLK_PAGE2 5

static T_CLK_MAP clk_page2[]=
  {
  {-1,INV_DESK,INV_DESK_Y,639,378,vls_click,2,-1},
  {-1,520,378,639,479,go_next_page,2,-1},
  {-1,54,378,497,479,view_another_click2,2,-1},
  {1,0,0,639,479,gen_exit_editor,8,-1},
  {0,0,0,639,479,empty_clk,0xff,-1},
  };
#define CLK_PAGE3 4

/*static T_CLK_MAP clk_page3[]=
  {
  {-1,520,378,639,479,go_next_page,2,-1},
  {-1,54,378,497,479,view_another_click2,2+8,-1},
  {0,0,0,639,479,edit_another_click2,8,-1},
  {0,0,0,639,479,empty_clk,0xff,-1},
  };
*/


#define H_GENERACE  (H_MENUS_FREE+100)
#define H_GEN_PERLA (H_MENUS_FREE+101)
#define H_GEN_TOPBAR (H_MENUS_FREE+102)
#define H_GEN_OKBUTT (H_MENUS_FREE+103)
#define H_GEN_CHARGEN (H_MENUS_FREE+104)
#define H_GEN_CHARGENB (H_MENUS_FREE+105)
#define H_GEN_CHARGENM (H_MENUS_FREE+106)
#define H_GEN_XICHTY (H_MENUS_FREE+107)
#define H_GEN_POSTAVY (H_GEN_XICHTY+MAX_XICHTS)

TDREGISTERS char_gen_reg[]=
    {
    {H_GENERACE,"postavy.pcx",pcx_15bit_decomp,SR_BGRAFIKA},
    {H_GEN_PERLA,"perla.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_GEN_TOPBAR,"topbar_p.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_GEN_CHARGEN,"chargen.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_GEN_CHARGENB,"chargenb.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_GEN_CHARGENM,"chargenm.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    };

#define BOTT 378
#define LEFT 520

char *b_texty[4];
char b_disables;

void displ_button(char disable,char **text)
  {
  int posy[]={0,18,37,55};
  int sizy[]={18,20,20,21};
  int i;

  int32_t scr_linelen2 = GetScreenPitch();

  set_font(H_FTINY,0);
  put_picture(LEFT,BOTT,ablock(H_GEN_CHARGEN));
  for(i=0;i<4;i++)
     {
     if (disable & 1)
        {
        put_8bit_clipped(ablock(H_GEN_CHARGENB),(392+posy[i])*scr_linelen2+524+GetScreenAdr(),posy[i],96,sizy[i]);
        font_color(color_butt_off);
        }
     else
        {
        font_color(color_butt_on);
        }
     disable>>=1;
     set_aligned_position(LEFT+50,BOTT+14+13+i*19,1,2,*text);
     outtext(*text);
     text++;
     }
  }
static void draw_other_bar()
  {
  int i;
  int32_t scr_linelen2 = GetScreenPitch();

  const word *bbar=ablock(H_BOTTBAR);
  word *screen=GetScreenAdr()+(480-102)*scr_linelen2;
  for (i=0;i<102;i++,screen+=scr_linelen2,bbar+=scr_linelen2) memcpy(screen,bbar,scr_linelen2*2);
  //put_8bit_clipped(ablock(H_GEN_OKBUTT),378*640+520+screen,0,120,102);
  displ_button(b_disables,b_texty);
  put_picture(0,0,ablock(H_GEN_TOPBAR));
  zobraz_staty(&cur_vls);
  }


static void display_character(THUMAN *p,char i)
  {
    const word *w;
 put_picture(4,SCREEN_OFFLINE,ablock(H_IOBLOUK));
 if (p->used)
     {
     w=ablock(H_GEN_POSTAVY+(p->xicht));
     put_picture(HUMAN_X+30,HUMAN_Y-w[1],w);
     }
  if (i) draw_other_bar();
  }
static void zobraz_error_text()
  {
  if (error_text!=NULL)
     {
     trans_bar(0,362,640,16,0);
     set_font(H_FONT6,RGB555(31,31,0));
     position(10,364);outtext(error_text);
     }
  }




static void vypocet_perly(int angle,int xp,int yp,int *x,int *y)
  {
  float xr,yr;
  float rad;

  rad=angle*3.14159265f/180;
  xr=(float)cos(rad)*PERLA_POLOMER;
  yr=(float)sin(rad)*PERLA_POLOMER;
  *x=(int)xr+PERLA_STRED_X;
  *y=PERLA_STRED_Y-(int)yr;
  x[0]-=xp>>1;
  y[0]-=yp>>1;
  }

static void zobraz_perlu(void)
  {
    const word *perla;
  int x,y;
  word *scr,*sss;
  char *p;
  const word *b;
  int xs,ys,xxs;

  int32_t scr_linelen2 = GetScreenPitch();

  alock(H_GEN_PERLA);
  perla=ablock(H_GEN_PERLA);
  if (pod_perlou==NULL)
     {
     pod_perlou=getmem(perla[0]*perla[1]*2+6);
     }
  vypocet_perly(cur_angle,perla[0],perla[1],&x,&y);
  xs=perla[0];ys=perla[1];
  get_picture(x,y,xs,ys,pod_perlou);
  sss=x+scr_linelen2*y+GetScreenAdr();
  p=(char *)(perla+256+3);
  b=perla+3;
  for(;ys>0;ys--)
     {
     scr=sss;
     for(xxs=0;xxs<xs;xxs++)
        if (!*p) scr++,p++;else
        if (*p<128) *scr++=b[(uint8_t)*p++];else
           {
           *scr=(*scr & RGB555(30,30,30))>>1;
           scr++;p++;
           }
     sss+=scr_linelen2;
     }
  //put_picture(x,y,perla);
  pod_perlou_x=x;
  pod_perlou_y=y;
  aunlock(H_GEN_PERLA);
  }

static void schovej_perlu(void)
  {
  put_picture(pod_perlou_x,pod_perlou_y,pod_perlou);
  }

static void vypocet_vlastnosti(int angle,T_VLASTS *vls)
  {
  T_VLASTS *low,*hi;
  div_t rm;
  int p;

  rm=div(angle,45);
  p=rm.quot;
  low=rohy+p;p++;if (p>=8) p=0;
  hi=rohy+p;
//  CALC_DIFF(8,12,44);
  vls->sill=CALC_DIFF(low->sill,hi->sill,rm.rem);
  vls->silh=CALC_DIFF(low->silh,hi->silh,rm.rem);
  vls->smgl=CALC_DIFF(low->smgl,hi->smgl,rm.rem);
  vls->smgh=CALC_DIFF(low->smgh,hi->smgh,rm.rem);
  vls->obrl=CALC_DIFF(low->obrl,hi->obrl,rm.rem);
  vls->obrh=CALC_DIFF(low->obrh,hi->obrh,rm.rem);
  vls->pohl=CALC_DIFF(low->pohl,hi->pohl,rm.rem);
  vls->pohh=CALC_DIFF(low->pohh,hi->pohh,rm.rem);
  vls->hpreg=CALC_DIFF(low->hpreg,hi->hpreg,rm.rem);
  vls->mpreg=CALC_DIFF(low->mpreg,hi->mpreg,rm.rem);
  vls->vpreg=CALC_DIFF(low->vpreg,hi->vpreg,rm.rem);
  low=&rohy[8];hi=vls;rm.rem=PERLA_POLOMER;
  vls->sill=CALC_DIFF2(low->sill,hi->sill,rm.rem);
  vls->silh=CALC_DIFF2(low->silh,hi->silh,rm.rem);
  vls->smgl=CALC_DIFF2(low->smgl,hi->smgl,rm.rem);
  vls->smgh=CALC_DIFF2(low->smgh,hi->smgh,rm.rem);
  vls->obrl=CALC_DIFF2(low->obrl,hi->obrl,rm.rem);
  vls->obrh=CALC_DIFF2(low->obrh,hi->obrh,rm.rem);
  vls->pohl=CALC_DIFF2(low->pohl,hi->pohl,rm.rem);
  vls->pohh=CALC_DIFF2(low->pohh,hi->pohh,rm.rem);
  vls->hpreg=CALC_DIFF2(low->hpreg,hi->hpreg,rm.rem);
  vls->mpreg=CALC_DIFF2(low->mpreg,hi->mpreg,rm.rem);
  vls->vpreg=CALC_DIFF2(low->vpreg,hi->vpreg,rm.rem);
  }

/*static void vypocet_statu(T_VLASTS *vls,T_STATY *sts)
  {
  sts->zivl=vls->sill*3/2;
  sts->zivh=vls->silh*3/2;
  sts->manl=vls->smgl*2;
  sts->manh=vls->smgh*2;
  sts->konl=vls->obrl*2;
  sts->konh=vls->obrh*2;
  sts->akc=(((vls->pohl+vls->pohh)/2)+14)/15;
  }
*/

static void zobraz_staty(T_VLASTS *st)
  {
  char s[100];

  set_font(H_FTINY,0);font_color(color_topbar);
  sprintf(s,texty[2],st->sill,st->silh);
  position(230,2);outtext(s);
  sprintf(s,texty[3],st->smgl,st->smgh);
  position(330,2);outtext(s);
  sprintf(s,texty[4],st->pohl,st->pohh);
  position(430,2);outtext(s);
  sprintf(s,texty[5],st->obrl,st->obrh);
  position(530,2);outtext(s);
  }


void redraw_generator(char show)
  {
  T_STATY z;
  int i;
  memset(&z,20,sizeof(z));
  schovej_mysku();
  put_picture(INV_DESK,SCREEN_OFFLINE,ablock(H_GENERACE));
  if (cur_xicht==-1)
     {
     put_picture(4,SCREEN_OFFLINE,ablock(H_IOBLOUK));
     bott_draw(1);
     draw_other_bar();
     }
  else display_character(postavy+cur_edited,1);
  zobraz_error_text();
  zobraz_perlu();
  for(i=0;i<MAX_XICHTS;i++)
     {
     if (disable[poradi[i]])
        trans_bar(28+INV_DESK+i*XICHT_STEP,18+INV_DESK_Y,26,38,0);
     }
  ukaz_mysku();
  if (show) showview(0,0,0,0);
  }


static void set_xicht(int player,int xicht)
  {
  THUMAN *h=postavy+player;
     h->used=1;
     h->xicht=xicht;
     h->vlastnosti[VLS_MAXHIT]=1;
     h->vlastnosti[VLS_KONDIC]=1;
     h->lives=1;
     h->kondice=0;
     h->mana=0;
     h->sektor=viewsector;
     h->groupnum=1;
     h->jidlo=1;
     h->voda=1;
     h->bonus=0;
     h->spell=0;
  }

static char select_xicht(int id,int xa,int ya,int xr,int yr)
  {
  int i,j,k;
  char s[20];

  i=xr/XICHT_STEP;
  j=xr%XICHT_STEP;
  if (j>27) return 0;
  k=poradi[i];
  if (disable[k]) return 1;
  memset(disable,0,sizeof(disable));
  set_xicht(cur_edited,cur_xicht=k);
  postavy[cur_edited].female=women[i];
  b_disables=0x6;
   if (was_enter) send_message(E_KEYBOARD,13);
  else
   {
   strcpy(postavy[cur_edited].jmeno,texty[160+i]);
   send_message(E_KEYBOARD,27);
   was_enter=0;
   }
  sprintf(s,XICHT_NAME,k);
  def_handle(H_XICHTY+cur_edited,s,pcx_8bit_decomp,SR_BGRAFIKA);
  sprintf(s,CHAR_NAME,k);
  def_handle(H_POSTAVY+cur_edited,s,pcx_8bit_decomp,SR_BGRAFIKA);
  for(j=0;j<POCET_POSTAV;j++) if (postavy[j].used) disable[postavy[j].xicht]=1;
  error_text=NULL;
  bott_draw(1);
  redraw_generator(1);
  displ_button(b_disables,b_texty);
  showview(0,0,0,0);
  id,xa,ya,xr,yr;
  return 1;
  }

static char vol_vlastnosti(int id,int xa,int ya,int xr,int yr)
  {
  int a;
  static char cancel=1;

  id,xa,ya,xr,yr;
  if (!ms_last_event.tl1)
     {
      cancel=1;
      return 0;
     }
  if (ms_last_event.event_type & 0x2) {
     if (xa<pod_perlou_x || ya<pod_perlou_y || xa>pod_perlou_x+pod_perlou[0] ||  ya>pod_perlou_y+pod_perlou[1] )
        {
        cancel=1;
        return 0;
        }
     else cancel=0;
  }
  if (cancel) return 0;
  xr=xa-PERLA_STRED_X;
  yr=-(ya-PERLA_STRED_Y);
  a=(int)(atan2((double)yr,(double)xr)*180/3.14159265);
  cur_polomer=(int)sqrt(xr*xr+yr*yr);
  if (cur_polomer>PERLA_MAXPOLOMER) cur_polomer=PERLA_MAXPOLOMER;
  xa=pod_perlou_x;
  ya=pod_perlou_y;
  schovej_mysku();
  schovej_perlu();
  if (a<0) a+=360;
  vypocet_vlastnosti(a,&cur_vls);
  cur_angle=a;
  zobraz_perlu();
  ukaz_mysku();
  update_mysky();
  showview(pod_perlou_x,pod_perlou_y,pod_perlou[0],pod_perlou[1]);
  showview(xa,ya,pod_perlou[0],pod_perlou[1]);
  put_picture(0,0,ablock(H_GEN_TOPBAR));
  zobraz_staty(&cur_vls);
  showview(250,0,640,17);
  return 1;
  }

static int edit_task=-1;

static void edit_name()
  {
  if (shut_downing_text) return;
  curcolor=0;bar32(120,2,120+104,16);
  edit_task=add_task(16384,type_text_v2,postavy[cur_edited].jmeno,120,2,104,
     sizeof(postavy[cur_edited].jmeno)-1,H_FONT6,RGB555(31,31,0),edit_name);
  }

static void stop_edit_name()
  {
  shut_downing_text=1;send_message(E_KEYBOARD,13);
  task_sleep();
  if (edit_task>0 && is_running(edit_task))
    term_task(edit_task);
  shut_downing_text=0;
  }

/*static char edit_another_click(int id,int xa,int ya,int xr,int yr)
  {
  short *w;

  id,xa,ya,xr,yr;
  w=ablock(H_OKNO);
  id=xr/w[0];
  if (!postavy[id].used) return 0;
  shut_downing_text=1;
  send_message(E_KEYBOARD,13);
  shut_downing_text=0;
  save_values[cur_edited][0]=cur_angle;
  save_values[cur_edited][1]=cur_polomer;
  select_player=cur_edited=id;
  cur_angle=save_values[cur_edited][0];
  cur_polomer=save_values[cur_edited][1];
  vypocet_vlastnosti(cur_angle,&cur_vls);
  cur_xicht=postavy[cur_edited].xicht;
  edit_name();
  bott_draw(1);
  redraw_generator();
  return 1;
  }
*/


static void def_entries()
  {
  int i;
  int s;
  TDREGISTERS *p;

  s=sizeof(char_gen_reg)/sizeof(TDREGISTERS);
  p=char_gen_reg;
  for(i=0;i<s;i++,p++) def_handle(p->h_num,p->name,p->proc,p->path);
  for(i=0;i<MAX_XICHTS;i++)
     {
     char s[15];

     sprintf(s,CHAR_NAME,i);
     def_handle(H_GEN_POSTAVY+i,s,pcx_8bit_decomp,SR_BGRAFIKA);
     mix_back_sound(0);
     }
  for(i=0;i<MAX_XICHTS;i++)
     {
     char s[15];

     sprintf(s,XICHT_NAME,i);
     def_handle(H_GEN_XICHTY+i,s,pcx_8bit_decomp,SR_BGRAFIKA);
     mix_back_sound(0);
     }
  }

static char go_next_page(int id,int xa,int ya,int xr,int yr)
  {
    const word *w;
  char *b;

  xa,ya;
  w=ablock(H_GEN_CHARGENM);
  b=(char *)(w+3+256);
  b+=*w*yr+xr;
  if (!*b) return 1;
  id=*b-1;
  if (b_disables & (1<<id)) return 1;
  send_message(E_CLOSE_GEN,id);
  return 1;
  }


void generuj_postavu(THUMAN *h)
  {
  short *v;
  h->used=1;
  h->groupnum=1;
  v=h->stare_vls;
  v[VLS_SILA]=DIFF_RAND(cur_vls.sill,cur_vls.silh);
  v[VLS_SMAGIE]=DIFF_RAND(cur_vls.smgl,cur_vls.smgh);
  v[VLS_POHYB]=DIFF_RAND(cur_vls.pohl,cur_vls.pohh);
  v[VLS_OBRAT]=DIFF_RAND(cur_vls.obrl,cur_vls.obrh);
  v[VLS_HPREG]=cur_vls.hpreg;
  v[VLS_MPREG]=cur_vls.mpreg;
  v[VLS_VPREG]=cur_vls.vpreg;
//  v[VLS_THIEF]=cur_vls.vpreg;
  h->lives=v[VLS_MAXHIT]=(v[VLS_SILA]*3+v[VLS_POHYB])/2;
  h->mana=v[VLS_MAXMANA]=v[VLS_SMAGIE]*2;
  h->kondice=v[VLS_KONDIC]=v[VLS_OBRAT]*2;
  v[VLS_OBRAN_L]=1;
  v[VLS_OBRAN_H]=2;
  v[VLS_UTOK_L]=1;
  v[VLS_UTOK_H]=2;
  prepocitat_postavu(h);
  h->inv_size=6;
  h->level=1;
  h->exp=0;
  h->bonus=5;
  h->jidlo=MAX_HLAD(h);
  h->voda=MAX_ZIZEN(h);
  h->mana_battery=32767;
  //postava je vygenerovana
  }

static void redraw_page3(char _)
  {
  update_mysky();
  schovej_mysku();
  curcolor=0;
  inv_display_vlastnosti();
  display_character(postavy+cur_edited,1);
  write_human_big_name(postavy[cur_edited].jmeno);
  draw_other_bar();
  displ_button(b_disables,b_texty);
  ukaz_mysku();
  showview(0,0,0,0);
  inv_redraw=redraw_page3;
  }


static void redraw_svitek(char _)
  {
  if (postavy[cur_edited].bonus==0)
     {
     char mode;
     mode=7;
     if (postavy[charmin-1].used) mode&=~2;
     if (!postavy[charmax-1].used) mode&=~1;
     if (!del_mode) mode&=~4;
     b_disables=mode;
     redraw_page3(0);
     return;
     }
  update_mysky();
  schovej_mysku();
  curcolor=0;
  bar32(0,16,30,16+360);
  bar32(620,16,640,16+360);
  inv_display_vlastnosti();
  display_character(postavy+cur_edited,0);
//  display_items_wearing(human_selected);
//  write_pocet_sipu();
//  display_rings();
  zobraz_error_text();
  displ_button(b_disables,b_texty);
  ukaz_mysku();
  showview(0,0,0,0);
  inv_redraw=redraw_svitek;
  }


static char validate_character(THUMAN *h)
  {
  if (h->jmeno[0]==0) error_text=texty[111];
  else error_text=NULL;
  return error_text==NULL;
  }

/*static char edit_another_click2(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya,xr,yr;
  cur_edited=select_player;
  schovej_mysku();
  edit_name();
  draw_other_bar();
  send_message(E_CLOSE_GEN,0);
  ukaz_mysku();
  update_mysky();
  return 1;
  }
 */
static void empty_proc(void)
  {
  }

char potvrzeno(char *text,void (*redraw)(char))
  {
  int i;
  unwire_proc=empty_proc;
  stop_edit_name();
  i=message(2,0,1,texty[118],text,texty[114],texty[115])==0;
  redraw(0);
  edit_name();
  return i;
  }

static char view_another_click2(int id,int xa,int ya,int xr,int yr)
  {
    const short *w;

  id,xa,ya,xr,yr;
  w=ablock(H_OKNO);
  id=xr/w[0];
  if (!(~b_disables & 0x3)) return 1;
  if (!postavy[id].used) return 0;
  shut_downing_text=1;send_message(E_KEYBOARD,13);
  shut_downing_text=0;
  select_player=id;
  bott_draw(1);
  human_selected=postavy+id;
  cur_edited=id;
  edit_name();
  redraw_page3(0);
  if (del_mode)
     if (potvrzeno(texty[117],redraw_page3))
     {
     del_mode=0;
     postavy[cur_edited].used=0;
     disable[postavy[cur_edited].xicht]=0;
     send_message(E_CLOSE_GEN,0);
     }
  return 0;
  }


static void wait_timer(EVENT_MSG *msg, void **udata) {
    if (msg->msg == E_TIMER) {
        exit_wait = 1;
    }
}

void effect_show(void)
  {
  int i;
  char s = exit_wait;
  schovej_mysku();
  for(i=0;i<12;i++)
     {
     showview(0,240-i*20-20,640,20);
     showview(0,240+(i*20),640,20);
     if (q_current_task() != -1) {
         task_wait_event(E_TIMER);
     } else {
        send_message(E_ADD, E_TIMER, wait_timer);
        escape();
        exit_wait = s;
        send_message(E_DONE, E_TIMER, wait_timer);
     }
     }
  ukaz_mysku();
  }


static void enter_reaction(EVENT_MSG *msg,void **unused)
  {
  unused;
  if (msg->msg==E_KEYBOARD)
     {
      int c = va_arg(msg->data, int);
      if ((c >> 1) == 1 || c == E_QUIT_GAME_KEY) {
          gen_exit_editor(2,0,0,0,0);
          return;
      } else if ((c & 0xFF)==13 && !shut_downing_text)
        {
        send_message(E_KEYBOARD,13);
        bott_draw(1);
        redraw_generator(1);
        msg->msg=-1;
        was_enter=1;
        }
     }
  }

static void enter_reaction2(EVENT_MSG *msg,void **_)
  {
  int c = va_arg(msg->data, int);
  if ((c >> 1) == 1 || c == E_QUIT_GAME_KEY) {
      gen_exit_editor(1,0,0,0,0);
      return;
  }
  if (msg->msg==E_KEYBOARD && (c & 0xFF)==13 && !shut_downing_text && ~b_disables & 0x3)
     {
     send_message(E_KEYBOARD,13);
     bott_draw(1);
     redraw_page3(0);
     msg->msg=-1;
     }
  }


char gen_exit_editor(int id,int xa,int ya,int xr,int yr)
  {
  xa,ya,xr,yr,id;

  if (del_mode==1)
     {
      del_mode=0;
      mouse_set_default(H_MS_DEFAULT);
      b_disables&=~0x4;
      redraw_svitek(0);
      return 1;
     }
  unwire_proc=empty_proc;
  shut_downing_text=1;send_message(E_KEYBOARD,13);shut_downing_text=0;
  if (message(2,0,1,texty[118],texty[113],texty[114],texty[115])==0)
     {
     send_message(E_CLOSE_GEN,255);
     }
  else
     {
      game_display_cancel_quit_request();
     if (id==2)redraw_generator(1);
     else redraw_svitek(0);
     edit_name();
     }
  return 1;
  }

char enter_generator()
  {
  int i;
  char rep=0;

  znova:
  del_mode=0;
  stop_edit_name();
  b_texty[0]=texty[170];
  b_texty[1]=texty[171];
  b_texty[2]=texty[172];
  b_texty[3]=texty[173];
  def_entries();
  curcolor=0;bar32(0,0,639,479);
  cur_angle=315;
  cur_edited=0;
  memset(postavy,0,sizeof(postavy));
  memset(disable,0,sizeof(disable));
  memset(save_values,0,sizeof(save_values));
  do
     {
     mouse_set_default(H_MS_DEFAULT);
     cur_angle=save_values[cur_edited][0];
     cur_polomer=save_values[cur_edited][1];
     set_xicht(cur_edited,cur_xicht=-1);
     select_player=-1;
     memset(&cur_stats,0,sizeof(cur_stats));
     vypocet_vlastnosti(cur_angle,&cur_vls);
     b_disables=0x7;
     redraw_generator(rep);if (!rep)effect_show();rep=1;
     edit_name();
     change_click_map(clk_page1,CLK_PAGE1);
     was_enter=0;
     do
       {
       send_message(E_ADD,E_KEYBOARD,enter_reaction);
       EVENT_MSG *ev= task_wait_event(E_CLOSE_GEN);;
       i=ev?va_arg(ev->data, int):255;
       send_message(E_DONE,E_KEYBOARD,enter_reaction);
       if (i==3 && potvrzeno(texty[116],redraw_generator)) goto znova;
       if (i==255) {
           term_task(edit_task);
           return 1;
       }
       }
     while (cur_xicht==-1 || i==3);
     send_message(E_KEYBOARD,13);
     save_values[cur_edited][0]=cur_angle;
     save_values[cur_edited][1]=cur_polomer;
     cur_xicht=0;
     error_text=NULL;
     generuj_postavu(postavy+cur_edited);
     human_selected=postavy+cur_edited;
     b_disables=0x7;
     do
       {
       redraw_svitek(0);
       change_click_map(clk_page2,CLK_PAGE2);
       do
        {
        send_message(E_ADD,E_KEYBOARD,enter_reaction2);
        EVENT_MSG *ev= task_wait_event(E_CLOSE_GEN);;
        i=va_arg(ev->data, int);
        send_message(E_DONE,E_KEYBOARD,enter_reaction2);
        if (i==3 && potvrzeno(texty[116],redraw_svitek)) goto znova;
        if (i==2)
           {
           del_mode=1;
           mouse_set_default(H_MS_WHO);
           b_disables|=0x4;
           redraw_svitek(0);
           i=3;
           }
        }
       while (i==3);
       mouse_set_default(H_MS_DEFAULT);
       del_mode=0;
       send_message(E_KEYBOARD,13);
       if (i==255) return 1;
       if (validate_character(postavy+cur_edited))
           for(cur_edited=0;postavy[cur_edited].used;cur_edited++);
       }
     while (error_text!=NULL);
     stop_edit_name();
     }
  while (i!=1);
  disable_click_map();
  schovej_mysku();
  curcolor=0;
  bar32(0,0,639,479);
  showview(0,0,0,0);
  ukaz_mysku();
  for(i=0;i<3;i++)
     {
     if (postavy[i].vlastnosti[VLS_SILA]>20) postavy[i].stare_vls[VLS_UTOK_H]++;
     if (postavy[i].vlastnosti[VLS_OBRAT]>20) postavy[i].stare_vls[VLS_OBRAN_H]++;
     }
  return 0;
  }









