#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>


#include <libs/types.h>
#include <libs/event.h>
#include <libs/memman.h>
#include <libs/devices.h>
#include <libs/bmouse.h>
#include <libs/bgraph.h>
#include <platform/sound.h>
#include <libs/strlite.h>
#include "engine1.h"
#include <libs/pcx.h>
#include "globals.h"
#include "version.h"

#include <string.h>
#define ZIVOTY_S 60
#define ZIVOTY_E 62
#define KONDIC_S 64
#define KONDIC_E 66
#define MANA_S   68
#define MANA_E   70
#define BARS_S   13
#define BARS_E   85
#define BARS_YS  (BARS_E-BARS_S)
#define PIC_X   3
#define PIC_Y   12
#define MANA_COLOR RGB555(15,0,31)
#define SEL_COLOR RGB555(31,31,31)

#define ZASAHB_X 8
#define ZASAHB_Y 20
#define ZASAHT_X 30
#define ZASAHT_Y 38


#define SHOW {swap_buffs();showview(0,0,0,0);swap_buffs();getche();`}

#define HUMAN_ADJUST 97

unsigned int barvy_skupin[POCET_POSTAV+1]=
         {
         RGB555(8,8,8)|FONT_TSHADOW,
         RGB555(31,28,00)|FONT_TSHADOW,
         RGB555(00,23,06)|FONT_TSHADOW,
         RGB555(31,11,13)|FONT_TSHADOW,
         RGB555(22,16,31)|FONT_TSHADOW,
         RGB555(28,13,31)|FONT_TSHADOW,
         RGB555(00,29,26)|FONT_TSHADOW
         };

char reverse_draw=0;
int viewsector=1,viewdir=1;
char norefresh=0,map_state=0;
int cur_sector; //sektor aktualni pozice
int back_color;
char global_anim_counter=0;
char one_buffer=0;
char set_halucination=0;
int hal_sector;
int hal_dir;
char see_monster=0;
char ghost_walls = 0;
char lodka=0;
char lodka_battle_draw = 0;  //jedncka pokud se zobrazuje programovani (pro lodku)
int bgr_distance=0; //vzdalenost pozadi od pohledu
int bgr_handle=0;

int spell_handle=0,spell_phase,spell_xicht;

THE_TIMER *bott_timer=NULL;
char *bott_text=NULL;
char bott_display;
char true_seeing=0;
char obl_anim_counter=0;
char obl_max_anim=1;
char anim_mirror=0;
static char showrune=0;
static int showruneitem=0;
static char lodka_loaded=0;



uint8_t dirs[10];
word minimap[VIEW3D_Z+1][VIEW3D_X*2+1];



char log_combat=0;



SPECTXT_ARR spectxtr;
static char spt_ptr=0;


void add_spectxtr(word sector,word fhandle,word count,word repeat,integer xpos)
  {
  SPECTXTR *sp=spectxtr+spt_ptr;

  sp->sector=sector;
  sp->handle=fhandle;
  sp->count=count;
  sp->pos=0;
  sp->repeat=repeat;
  sp->xpos=xpos;
  if (count)
     if ((++spt_ptr)>=MAX_SPECTXTRS) spt_ptr=0;
  map_coord[sector].flags |= MC_SPECTXTR;
  }

void calc_spectxtrs(void)
  {
  int i;

  for(i=0;i<MAX_SPECTXTRS;i++) if (spectxtr[i].repeat)
     {
     SPECTXTR *sp=spectxtr+i;
     if (++sp->pos>=sp->count)
        {
        sp->pos=0;if (sp->repeat>0) sp->repeat--;
        }
     }
  }

static void draw_spectxtrs(word sector,int celx,int cely)
  {
  int i;
  char a=1;

  if (!(map_coord[sector].flags & MC_SPECTXTR)) return;
  for(i=0;i<MAX_SPECTXTRS;i++) if (spectxtr[i].repeat && spectxtr[i].sector==sector)
     {
     SPECTXTR *sp=spectxtr+i;

     draw_spectxtr(ablock(sp->handle+sp->pos),celx,cely,sp->xpos);
     a=0;
     }
  if (a)
     map_coord[sector].flags &= ~MC_SPECTXTR;
  }

void init_spectxtrs(void)
  {
  memset(spectxtr,0,sizeof(spectxtr));
  }

void ukaz_kompas(char mode)
  {
  static int phase=0,speed=0;
  int direct;

  if (mode==1)
     {
     direct=(3-viewdir);
     direct=phase-direct*10;
     if (direct<-20) direct+=40;
     if (direct>=20) direct-=40;
     speed=((direct>0)-(direct<0));
     }
  if (mode!=255) put_image(ablock(H_KOMPAS),GetScreenAdr()+476,phase*16,102,16);
  if (mode==1)
     {
     phase-=speed;if (phase>39) phase=0;
     if(phase<0) phase=39;
     }
  if (mode==255) showview(476,0,102,15);
  }

void show_money()
  {
  char c[20];
  set_font(H_FONT7,RGB555(28,28,21));
  sprintf(c,"%d",money);
  set_aligned_position(460,13,2,2,c);
  outtext(c);
  }


void anim_sipky(int h,int mode)
  {
   int32_t scr_linelen2 = GetScreenPitch();
   static int phase=0;
  static char drw=0;
  static short handle=0;

     if (mode==1)
       {
       handle=h;
       phase=6;
       return;
       }
    if (phase && mode!=255)
       {
       int i;

       if (phase>4) i=8-phase; else i=phase;
       i=(i-2)*110;
       if (i>=0)
       put_8bit_clipped(ablock(handle),GetScreenAdr()+378*scr_linelen2+498,i,142,102);
       else put_picture(498,378,ablock(H_SIPKY_END));
       drw=1;
       if (mode!=-1)
        {
        phase--;
        }
       }
  if (mode==255 && drw)
     {
     showview(498,378,142,102);
     drw=0;
     }
  }

void chveni(int i)
  {
  static int pos=0;
  static int count=0;

  if (!count && !i) return;
  if (i) count=i;
  count--;
  if (!count) pos=0;
  setvesa_displaystart(8*pos,0);
  pos=!pos;
  }

void objekty_mimo(THE_TIMER *t)
  {
    if (cur_mode == MD_END_GAME) return;
  schovej_mysku();
  ukaz_kompas(1);
  anim_sipky(0,0);
  ukaz_mysku();
  ukaz_kompas(255);
  anim_sipky(0,255);
  chveni(0);
  }

void draw_blood(char mode,int mob_dostal,int mob_dostal_pocet)
	{
	static int phase=100;
	static int block;
	static int dostal;
	char s[20];
	word *adr;
	int i;

	int32_t scr_linelen2 = GetScreenPitch();
	mob_dostal_pocet;
  if (mode) {
    if (phase<3) dostal+=mob_dostal_pocet;else dostal=mob_dostal_pocet;
    phase=0;
    block=mob_dostal;
    return;
    }
	if (phase>7) return;
	adr=520+378*scr_linelen2+GetScreenAdr();
	i=105*phase;
	phase++;
  put_8bit_clipped(ablock(H_KREVMIN+block-1),adr,i,120,102);
	set_font(H_FTINY,RGB555(31,31,31));
	int2ascii(dostal,s,10);
	set_aligned_position(60+520,51+378,1,1,s);outtext(s);
	}

word *bott_clear(void)
  {
  word *bott_scr;
  int32_t scr_linelen2 = GetScreenPitch();

  int32_t sz;
  bott_scr=(word *)getmem(sz = scr_linelen2*104*2);
  memset(bott_scr,0,sz);
  return bott_scr;
  }

static void draw_small_icone(int num,int x,int y)
  {
  const word *pic;

  pic=ablock(H_POSTUP);
  num*=13;x+=num;
  put_textured_bar(pic,x,y,13,13,num,0);
  }

static const void *bott_fletna_normal(const void *pp, int32_t *s)
  {
  word *bott_scr;
  int i,x;
  bott_scr=bott_clear();
  RedirectScreen(bott_scr);
  put_picture(0,0,ablock(H_FLETNA_BAR));
  set_font(H_FTINY,RGB555(31,31,0));
  set_aligned_position(103,52,1,1,texty[174]);outtext(texty[174]);
  for(i=0,x=156;i<12;i++,x+=22)
     {
     set_aligned_position(x,32,1,1,texty[180+i]);outtext(texty[180+i]);
     }
  void *out=GetScreenAdr();
  *s=GetScreenSizeBytes();
  RestoreScreen();
  return out;
  }

static const void *bott_draw_normal(const void *pp, int32_t *s)
  {
  int i,j;int x,xs=0,y;
  word *bott_scr;
  THUMAN *p;

  int32_t scr_linelen2 = GetScreenPitch();
  bott_scr=bott_clear();
  RedirectScreen(bott_scr);
  if (battle && cur_mode==MD_INBATTLE) put_picture(0,0,ablock(H_BATTLE_BAR));
  else put_picture(0,0,ablock(H_DESK));
  memcpy(&xs,ablock(H_OKNO),2);
  x=54;
  for(j=0;j<POCET_POSTAV;j++)
     if ((p=&postavy[i=group_sort[j]])->used)
     if (cur_mode!=MD_PRESUN || i==moving_player)
     {
     char c[]=" ";int z,lv,llv;
     put_picture(x,0,ablock(H_OKNO));lv=p->lives;llv=p->vlastnosti[VLS_MAXHIT];
     if (lv || p->used & 0x80)
        {
        z=3-((lv-1)*4/llv);if (lv==llv) z=0;
        z*=75;
        if (p->xicht>=0)put_8bit_clipped(ablock(H_XICHTY+i),bott_scr+PIC_X+x+PIC_Y*scr_linelen2,z,54,75);
       /* if (!lv && !(p->used & 0x80) && p->kondice>0) {
          greyscale_rectangle(PIC_X+x,PIC_Y, 54, 75);
        }*/
        if (p->bonus) draw_small_icone(0,PIC_X+x+1,PIC_Y+1);
        if (p->spell) draw_small_icone(1,PIC_X+x+1,PIC_Y+1);
        if (!p->voda) draw_small_icone(2,PIC_X+x+1,PIC_Y+1);
        if (!p->jidlo) draw_small_icone(3,PIC_X+x+1,PIC_Y+1);
        }
     else put_picture(PIC_X+x,PIC_Y,ablock(H_LEBKA));
     curcolor=0;
     y=BARS_YS-p->lives*BARS_YS/p->vlastnosti[VLS_MAXHIT];
     if (y) bar32(x+ZIVOTY_S,BARS_S,x+ZIVOTY_E,BARS_S+y);
     y=BARS_YS-p->kondice*BARS_YS/p->vlastnosti[VLS_KONDIC];
     if (y) bar32(x+KONDIC_S,BARS_S,x+KONDIC_E,BARS_S+y);
     if (p->vlastnosti[VLS_MAXMANA]) y=BARS_YS-p->mana*BARS_YS/p->vlastnosti[VLS_MAXMANA];else y=BARS_YS;
     if (y<0) y=0;
     if (y) bar32(x+MANA_S,BARS_S,x+MANA_E,BARS_S+y);
     if (p->sektor!=viewsector || p->inmaphash != current_map_hash) trans_bar25(x,0,74,102);
     set_font(H_FLITT,p->groupnum==cur_group && !battle?SEL_COLOR:barvy_skupin[p->groupnum]);
     set_aligned_position(x+36,92,1,0,p->jmeno);outtext(p->jmeno);
     c[0]=p->inmaphash !=current_map_hash?'-':p->groupnum+48;set_aligned_position(x+5,86,0,2,c);outtext(c);
     if (cur_mode==MD_INBATTLE && p->inmaphash == current_map_hash)
        {
        char s[20];
		signed char dir;
        set_font(H_FBOLD,RGB555(31,31,0));
        sprintf(s,texty[40],p->actions);
        set_aligned_position(x+56,86,2,2,s);outtext(s);
		dir=viewdir-p->direction;
		if (abs(dir)==2) c[0]=2;
		else if (dir==-1 || dir==3) c[0]=1;
		else if (dir==-3 || dir==1) c[0]=3;
		  if (dir)
		  {
   			set_font(H_FSYMB,p->groupnum==cur_group && !battle?SEL_COLOR:barvy_skupin[p->groupnum]);
			c[0]+=4;set_aligned_position(x+10,86,0,2,c);outtext(c);
		  }
        }
     if (i==select_player) rectangle(x+3,12,x+3+54,12+75,SEL_COLOR);
     if (p->dostal)
        {
        char s[20];
        put_picture(x+ZASAHB_X,ZASAHB_Y,ablock(H_PZASAH));
        set_font(H_FBOLD,RGB555(31,31,0));
        sprintf(s,"%d",p->dostal);
        set_aligned_position(x+ZASAHT_X,ZASAHT_Y,1,1,s);outtext(s);
        }
     x+=xs;
     }
  if (cur_mode==MD_PRESUN || cur_mode==MD_UTEK)
     {
     char s[40];
     set_font(H_FBOLD,RGB555(31,31,16));
     position(150,20);outtext(texty[cur_mode==MD_PRESUN?42:44]);
     sprintf(s,texty[60+cislovka(postavy[moving_player].actions)],postavy[moving_player].actions);
     position(150,35);outtext(s);
     position(150,50);outtext(texty[63]);
     }
/*  if (mob_dostal)
        {
        word *w;
        char s[40];

        w=ablock(H_MZASAH1+mob_dostal-1);
        put_picture(580-(w[0]>>1),55-(w[1]>>1),w);
        itoa(mob_dostal_pocet,s,10);
        set_font(H_FLITT5,0xffff);
        set_aligned_position(580,55,1,1,s);
        outtext(s);
        }
 */
  void *out = GetScreenAdr();
  *s=GetScreenSizeBytes();
  RestoreScreen();
  return out;
  }


void bott_timer_draw(struct the_timer *q)
  {
  q;
  bott_display=BOTT_NORMAL;
  zneplatnit_block(H_BOTTBAR);
  free(bott_text);
  bott_text=NULL;
  bott_timer=NULL;
  }

const void *bott_disp_text_proc(const void *pp, int32_t *ss)
  {
  char *p,*text;
  int y=20;
  int xx,yy;

  text=alloca(strlen(bott_text)+2);
  set_font(H_FBOLD,NOSHADOW(0));
  zalamovani(bott_text,text,390,&xx,&yy);
  RedirectScreen(bott_clear());
  if (battle && cur_mode==MD_INBATTLE) put_picture(0,0,ablock(H_BATTLE_BAR));
  else put_picture(0,0,ablock(H_DESK));
  create_frame(70,20,400,50,1);
  p=text;
  do
     {
     position(70,y);
     outtext(p);
     y+=text_height(p);
     p=strchr(p,0)+1;
     }
  while (p[0]);
  void *out = GetScreenAdr();
  *ss=GetScreenSizeBytes();
  RestoreScreen();
  return out;
  }

void bott_disp_text(const char *text)
  {
  if (text==0) text="Chybi popisek!!";
  zneplatnit_block(H_BOTTBAR);
  if (bott_timer==NULL)
     bott_timer=add_to_timer(TM_BOTT_MESSAGE,MSG_DELAY,1,bott_timer_draw);
     else bott_timer->counter=MSG_DELAY;
  bott_display=BOTT_TEXT;
  if (bott_text!=NULL) free(bott_text);
  bott_text=(char *)getmem(strlen(text)+1);
  strcpy(bott_text,text);
  }

/*
static void MaskPutPicture(int x, int y, char mask, word color, char blend, const void *pic)
  {
  short *info=(short *)pic;
  char *data=(char *)(info+3+256);
  int32_t scr_linelen2 = GetScreenPitch();
  word *pos=GetScreenAdr()+x+y*scr_linelen2;
  if (blend) color=color & 0x7BDE;
  for (y=0;y<info[1];y++,pos+=scr_linelen2,data+=info[0])
	for (x=0;x<info[0];x++)
	  if (data[x]==mask) {
		if (blend) {
		  pos[x]=((pos[x] & 0x7BDE)+color)>>1;
		} else {
		  pos[x]=color;
		}
	  }
  }
  */

static void *extractByMask(const short *image, const short *mask_data, int mask) {
    short left = 0x7FFF;
    short right = 0;
    short top = 0x7FFF;
    short bottom =0;
    short xs = image[0];
    short mxs = mask_data[0];
    short mys = mask_data[1];
    const char *mdata = (const char *)mask_data+sizeof(short)*3+512;
    for (short y = 0; y < mys; ++y) {
        for (short x = 0; x < mxs; ++x) {
            if (mdata[y*mxs+x] == mask) {
                if (left > x) left = x;
                if (right < x) right = x;
                if (bottom < y) bottom = y;
                if (top > y) top = y;
            }
        }
    }
    if (left > right || top > bottom) return NULL;
    short nxs = right-left+1;
    short nys = bottom-top+1;
    short *newimg = getmem(nxs*nys*2+3+256);
    newimg[0] = nxs;
    newimg[1] = nys;
    newimg[2] = image[2];
    memcpy(newimg+2, image+2, sizeof(short)*266);
    char *imgdata = (char *)(newimg+3+256);
    const char *srcimgdata = (char *)(image+3+256);
    for (short y = top; y <= bottom; ++y) {
        for (short x = left; x <= right; ++x) {
            char color = 0;
            if (mdata[y*mxs+x] == mask) {
                color = srcimgdata[y*xs+x];
            } else {
                color = 0;
            }
            imgdata[(y - top)*nxs+(x - left)] = color;
        }
    }
    return newimg;


}


const void *bott_draw_rune(const void *pp, int32_t *ss)
  {
  int sel_zivel=showrune/10;
  int sel_rune=showrune%10;
//  int maskrune=runes[sel_zivel];
//  int i;
  char buff[300];
  int spell=(sel_zivel*7+sel_rune)*3;
  RedirectScreen(bott_clear());
  if (battle && cur_mode==MD_INBATTLE) put_picture(0,0,ablock(H_BATTLE_BAR));
  else put_picture(0,0,ablock(H_DESK));

  short *rn = extractByMask(ablock(H_RUNEBAR1+sel_zivel),
              ablock(H_RUNEMASK), sel_rune+6);
/*

  put_picture(520,0,);
  for (i=0;i<7;i++)
	if (!(maskrune & (1<<i))) MaskPutPicture(520,0,i+6,0,0,ablock(H_RUNEMASK));
	else if (i!=sel_rune) MaskPutPicture(520,0,i+6,0,1,ablock(H_RUNEMASK));
  if (sel_zivel) trans_bar(520,0,sel_zivel*24,22,0);
  if (sel_zivel!=4)trans_bar(520+24+sel_zivel*24,0,96-sel_zivel*24,22,0);
  */
  create_frame(70,20,400,50,1);
  set_font(H_FBOLD,NOSHADOW(0));
  position(120,30);
  outtext(glob_items[showruneitem].jmeno);
  sprintf(buff,"%s %d, %s %d",texty[11],get_spell_um(spell),texty[16],get_spell_mana(spell));
  position (120,60);
  outtext(buff);
  if (rn) {
      put_picture(80,35,rn);
      free(rn);
  }
  void *out = GetScreenAdr();
  *ss=GetScreenSizeBytes();
  RestoreScreen();
  return out;
  }

void bott_disp_rune(char rune, int item)
  {
  showrune=rune;
  showruneitem=item;
  bott_display=BOTT_RUNA;
  zneplatnit_block(H_BOTTBAR);
  if (bott_timer==NULL)
     bott_timer=add_to_timer(TM_BOTT_MESSAGE,MSG_DELAY,1,bott_timer_draw);
     else bott_timer->counter=MSG_DELAY;

  }

void bott_text_forever()
  {
  delete_from_timer(TM_BOTT_MESSAGE);
  bott_timer=NULL;
  }

const void *bott_draw_proc(const void *p, int32_t *s, int h)
  {
  switch (bott_display)
     {
     case BOTT_NORMAL:return bott_draw_normal(p,s);break;
     case BOTT_TEXT:return bott_disp_text_proc(p,s);break;
     case BOTT_FLETNA:return bott_fletna_normal(p,s);break;
	 case BOTT_RUNA:return bott_draw_rune(p,s);break;
	 default: return p;
     }
  }

void bott_draw(char priority)
  {
  if (priority)
     {
     if (bott_display!=BOTT_NORMAL)
        {
        delete_from_timer(TM_BOTT_MESSAGE);
        bott_timer=NULL;
        bott_display=BOTT_NORMAL;
        }
     }
  if (bott_display==BOTT_NORMAL)
     zneplatnit_block(H_BOTTBAR);
  }

void bott_draw_fletna()
  {
  bott_display=BOTT_FLETNA;
     zneplatnit_block(H_BOTTBAR);
  }

void draw_spell(int handle,int phase,int xicht)
  {
  int x,y,i;
  const word *w;
  int32_t scr_linelen2 = GetScreenPitch();


  if (bott_display!=BOTT_NORMAL) bott_draw(1);
  for(i=0;i<POCET_POSTAV;i++)
     if (xicht & (1<<group_sort[i]))
     {
     w=ablock(H_OKNO);
     x=54+w[0]*i+PIC_X;
     y=PIC_Y+378;
     put_8bit_clipped(ablock(handle),GetScreenAdr()+x+y*scr_linelen2,phase*75,54,75);
     neco_v_pohybu=1;
     }
  }

void other_draw()
  {
//  if (cancel_render) return;
  StripBlt(ablock(H_BOTTBAR),480-102,102);
//  memcpy(GetScreenAdr()+(480-102)*scr_linelen2,ablock(H_BOTTBAR),scr_linelen2*102*2);
  if (spell_handle)
     {
     draw_spell(spell_handle,spell_phase,spell_xicht);
     spell_phase++;
     if (spell_phase>=10)
        {
        spell_handle=0;
        spell_xicht=0;
        }
     }
  put_picture(0,0,ablock(1));
  ukaz_kompas(0);
  show_money();
  anim_sipky(0,-1);
  draw_fx();
  draw_console_window();
  int32_t scr_linelen2 = GetScreenPitch();

  memset(GetScreenAdr()+(SCREEN_OFFLINE-1)*scr_linelen2,0,1280);
  memset(GetScreenAdr()+(SCREEN_OFFLINE+360)*scr_linelen2,0,1280);
  }

void display_spell_in_icone(int handle,int xicht)
  {
  spell_phase=0;
  spell_handle=handle;
  spell_xicht|=xicht;
  }

int fc_num(int anim_counter,int sector,char floor)
  {
  int mode;
  TSECTOR *s;
  TMAP_EDIT_INFO *m;
  int basic;

  s=&map_sectors[sector];
  m=&map_coord[cur_sector];
  if (floor)
     {
     basic=s->floor;
     mode=s->flags & 0xf;
     }
    else
     {
     basic=s->ceil;
     mode=s->flags>>4;
     }

  if (mode & 0x8)
     return basic+(anim_counter % ((mode & 0x7)+1));
  switch (mode)
     {
     case 0: return basic;
     case 1: return basic+((m->x+m->y+dirs[1]) & 0x1);
     case 2: return basic+(dirs[1] & 0x1);
     case 3: return basic+(dirs[1] & 0x1)*2+((m->x+m->y+(dirs[1]>>1)) & 0x1);
     case 4: return basic+dirs[1];
     case 5: return basic+dirs[1]*2+((m->x+m->y+(dirs[1]>>1)) & 0x1);
     }
  return basic;
  }

int enter_tab[VIEW3D_Z+1][VIEW3D_X*2+1]=
  {{4,3,3,3,3,3,3,3,4},
   {4,3,3,2,3,2,3,3,4},
   {4,3,3,2,4,2,3,3,4},
   {4,3,1,1,4,1,1,3,4},
   {4,1,1,1,4,1,1,1,4}};



void crt_minimap_itr(int sector,int smer,int itrx,int itry, int automap)
  {
  static int sector_temp;
  static int32_t sideflags;
  static short enter=0;
  short savee;

  if (itrx==VIEW3D_X && itry==0) enter=0;
  if (itrx<0 || itrx>VIEW3D_X*2 || itry>VIEW3D_Z) return;
  if (minimap[itry][itrx]) return;
  minimap[itry][itrx]=sector;
  if (!set_halucination) map_coord[sector].flags|=automap & (itrx<=VIEW3D_X+1 && itrx>=VIEW3D_X-1)  ;
  if (itrx<=VIEW3D_X)
     {
     sector_temp=map_sectors[sector].step_next[dirs[0]];
     sideflags=map_sides[sector*4+dirs[0]].flags;
     if (sector_temp && sideflags & 0x80 && enter>=0)
        {
        savee=enter;
        enter=-enter_tab[itry][itrx];
        crt_minimap_itr(sector_temp,smer,itrx-1,itry,automap &(sideflags & 1));
        enter=savee;
        }
     }
  if (itrx>=VIEW3D_X)
     {
     sector_temp=map_sectors[sector].step_next[dirs[2]];
     sideflags=map_sides[sector*4+dirs[2]].flags;
     if (sector_temp && sideflags & 0x80 && enter<=0)
        {
        savee=enter;
        enter=enter_tab[itry][itrx];
        crt_minimap_itr(sector_temp,smer,itrx+1,itry,automap &(sideflags & 1));
        enter=savee;
        }
     }
    sector_temp=map_sectors[sector].step_next[dirs[1]];
     sideflags=map_sides[sector*4+dirs[1]].flags;
  if (sector_temp && sideflags & 0x80)
     {
     savee=enter;
     enter-=(enter>0)-(enter<0);
     crt_minimap_itr(sector_temp,smer,itrx,itry+1,automap &(sideflags & 1));
     enter=savee;
     }
  }


void create_minimap(int sector, int smer)
  {
  memset(minimap,0,sizeof(minimap));
  dirs[1]=smer;
  dirs[0]=(smer-1) & 3;
  dirs[2]=(smer+1) & 3;
  crt_minimap_itr(sector,smer,VIEW3D_X,0,1);
  }





static const void *check_autofade(const void *image, char ceil, int dark)
{
    const char *data=image;
  if (data[5]==0x80)
  {
    word *xy=(word *)image;
	if (mglob.map_autofadefc==1)
	{
	  word *imgdata=xy+3;
	  int br=back_color>>10;
	  int bg=(back_color>>5) & 0x1F;
	  int bb=back_color & 0x1F;
	  int y;

      if (dark) br=bg=bb=0;

	  for(y=0;y<xy[1];y++)
	  {
		float factor=(float)y/(xy[1]-1);
		int x;
		if (!ceil) factor=1.0f-factor;
		factor=factor*factor*factor;
		for (x=0;x<xy[0];x++)
		{
		  int r=*imgdata>>10;
		  int g=(*imgdata>>5) & 0x1F;
		  int b=*imgdata & 0x1F;
		  r=(int)(r+factor*(br-r));
		  g=(int)(g+factor*(bg-g));
		  b=(int)(b+factor*(bb-b));
		  *imgdata=(r<<10)|(g<<5)|b;
		  imgdata++;
		}
	  }
	}
    xy[2]=xy[2] & 0xFF;
  }
  return image;
}

#define draw_floor(s,celx,cely,dark) if (s->floor) draw_floor_ceil(celx,cely,0,check_autofade(ablock(num_ofsets[FLOOR_NUM]+fc_num(global_anim_counter,sector,1)),0,dark));
#define draw_ceil(s,celx,cely,dark) if (s->ceil)  draw_floor_ceil(celx,cely,1,check_autofade(ablock(num_ofsets[CEIL_NUM]+fc_num(global_anim_counter,sector,0)),1,dark));
#define GET_OBLOUK(p) ((p->oblouk & 0xf)+(p->prim_anim>>4))

static int left_shiftup,right_shiftup;

int draw_basic_floor(int celx,int cely,int sector)
  {
  TSECTOR *s;
  int dark;

  s=&map_sectors[sector];
  dark=map_coord[sector].flags & MC_SHADING;
  draw_floor(s,celx,cely,dark);
  draw_ceil(s,celx,cely,dark);
  return 0;
  }

static int calc_item_shiftup(TITEM *it)
  {
    const short *s;
  char *c;
  int y,x,xs,ys,t;

  t=0;
  s=ablock(it->vzhled+face_arr[0]);
  xs=s[0];ys=s[1];c=((char *)s)+PIC_FADE_PAL_SIZE;
  c+=xs*ys-1;y=0;
  while (y<ys && t<254)
     {
     x=0;
     while(x<xs) if (*c!=0) return t;else c--,x++;
     y++;t++;
     }
  return t;
  }

void draw_vyklenek(int celx,int cely,int sector,int dir)
  {
  int i,j;
  TVYKLENEK *v;

  for(i=0;i<vyk_max;i++) if (map_vyk[i].sector==sector && map_vyk[i].dir==dir) break;
  if (i==vyk_max) return;
  v=map_vyk+i;
  for(i=0;(j=v->items[i])!=0;i++)
     {
     TITEM *it=&glob_items[j-1];
     if (it->shiftup==0xff) it->shiftup=calc_item_shiftup(it);
     draw_item2(celx,cely,v->xpos,v->ypos-it->shiftup,ablock(it->vzhled+face_arr[0]),i);
     }
  }


static int draw_basic_sector(int celx, int cely, int sector) {
    TSTENA *w, *q;
    int obl;

    int tmask = true_seeing?SD_TRUESEE:0;


    w = &map_sides[sector * 4];
    q = &w[dirs[1]];
    obl = GET_OBLOUK(q);
    if (cely < VIEW3D_Z) {
        if (q->flags & SD_LEFT_ARC && obl)
            show_cel2(celx, cely, ablock(num_ofsets[OBL_NUM] + obl), 0, 0, 1, ghost_walls );
        if (q->flags & SD_RIGHT_ARC && q->oblouk & 0x0f)
            show_cel2(celx, cely, ablock(num_ofsets[OBL2_NUM] + obl), 0, 0, 2, ghost_walls);
        if (q->flags & (SD_PRIM_VIS|tmask) && q->prim)
            show_cel2(celx, cely,
                    ablock(
                            num_ofsets[MAIN_NUM] + q->prim
                                    + (q->prim_anim >> 4)), 0, 0,
                    1 + (q->oblouk & SD_POSITION), ghost_walls | (q->flags & tmask));
        if (q->flags & (SD_SEC_VIS|tmask) && q->sec) {
            if (q->side_tag & SD_SHIFTUP) {
                if (cely != 0) {
                    show_cel2(celx, cely - 1,
                            ablock(
                                    num_ofsets[MAIN_NUM] + q->sec
                                            + (q->sec_anim >> 4)), 0, 0, 1, ghost_walls | (q->flags & tmask));
                }
            } else {
                show_cel2(celx, cely,
                        ablock(
                                num_ofsets[MAIN_NUM] + q->sec
                                        + (q->sec_anim >> 4)), q->xsec << 1,
                        q->ysec << 1, 0, ghost_walls | (q->flags & tmask));
            }
        }
        if (q->oblouk & 0x10)
            draw_vyklenek(celx, cely, sector, dirs[1]);
    }
    if (celx <= 0) {
        q = &w[dirs[0]];
        if (left_shiftup)
            show_cel(celx, cely, ablock(num_ofsets[LEFT_NUM] + left_shiftup), 0,
                    0, 2, ghost_walls), left_shiftup = 0;
        if (q->flags & (SD_PRIM_VIS|tmask) && q->prim )
            show_cel(-celx, cely,
                    ablock(
                            num_ofsets[LEFT_NUM] + q->prim
                                    + (q->prim_anim >> 4)), 0, 0,
                    2 + (q->oblouk & SD_POSITION), ghost_walls | (q->flags & tmask));
        if (q->flags & (SD_SEC_VIS|tmask) && q->sec) {
            if (q->side_tag & SD_SHIFTUP) {
                if (celx != 0) {
                    left_shiftup = q->sec + (q->sec_anim >> 4);
                } else {
                    left_shiftup = 0;
                }
            } else if (q->flags & SD_SPEC) {
                show_cel(celx, cely,
                        ablock(
                                num_ofsets[LEFT_NUM] + q->sec
                                        + (q->sec_anim >> 4)), 0, 0, 2, ghost_walls | (q->flags & tmask));
            } else {
                show_cel(celx, cely,
                        ablock(
                                num_ofsets[LEFT_NUM] + q->sec
                                        + (q->sec_anim >> 4)), q->xsec << 1,
                        q->ysec << 1, 0, ghost_walls | (q->flags & tmask));
            }
        }
    }
    if (celx >= 0) {
        q = &w[dirs[2]];
        if (right_shiftup)
            show_cel(celx, cely, ablock(num_ofsets[RIGHT_NUM] + right_shiftup),
                    0, 0, 3, ghost_walls), right_shiftup = 0;
        if (q->flags & (SD_PRIM_VIS|tmask) && q->prim )
            show_cel(celx, cely,
                    ablock(
                            num_ofsets[RIGHT_NUM] + q->prim
                                    + (q->prim_anim >> 4)), 0, 0,
                    3 + (q->oblouk & SD_POSITION), ghost_walls | (q->flags & tmask));
        if (q->flags & (SD_SEC_VIS|tmask) && q->sec) {
            if (q->side_tag & SD_SHIFTUP) {
                if (celx != 0)
                    right_shiftup = q->sec + (q->sec_anim >> 4);
                else
                    right_shiftup = 0;
            } else if (q->flags & SD_SPEC) {
                show_cel(celx, cely,
                        ablock(
                                num_ofsets[RIGHT_NUM] + q->sec
                                        + (q->sec_anim >> 4)), 0, 0, 3, ghost_walls | (q->flags & tmask));
            } else {
                show_cel(celx, cely,
                        ablock(
                                num_ofsets[RIGHT_NUM] + q->sec
                                        + (q->sec_anim >> 4)),
                        500 - (q->xsec << 1), q->ysec << 1, 1, ghost_walls | (q->flags & tmask));
            }
        }

    }
    return 0;
}

static int draw_lodku(int celx,int cely)
  {
  if (cely==0) return 1;
  show_cel2(celx,cely-1,ablock(H_LODKA0+(global_anim_counter & 7)),250,80,0,0);
  return 0;
  }


int p_place_table[4][5]=
     {
     {0,1,4,2,3},
     {1,2,4,3,0},
     {2,3,4,0,1},
     {3,0,4,1,2}
     };
int p_positions_x[5]={-32,32,32,-32,0};
int p_positions_y[5]={32,32,-32,-32,0};

void draw_players(int sector,int dir,int celx,int cely)
  {

  if (map_coord[sector].flags & MC_DPLAYER)
     {
     int i;
     THUMAN *p;
     char freep[5]= {0,0,0,0,0};
     int j,d,pp=0,f=0;

     memset(freep,0,sizeof(freep));
     for(i=0,pp=0;i<POCET_POSTAV;i++)
        {
        if ((p=postavy+i)->sektor==sector && p->inmaphash == current_map_hash && ((!p->lives && p->groupnum==0) || p->sektor!=viewsector))
           {
           pp++;
           d=(p->direction-dir)&0x3;
           if (p->lives)
                   for(j=0;j<5 && freep[p_place_table[d][j]];j++);
           else
                   for(j=4;j>=0 && freep[p_place_table[d][j]];j--);
           if (j==5 || j==-1) break;
           freep[f=p_place_table[d][j]]=i+1;
           }
        }
        if (pp == 1 && freep[f] & 1) {
            pp = (f + 1) & 3;
            freep[pp] = freep[f];
            freep[f] = 0;
        }
        for (i = 0; i < 5; i++)
            if ((j = freep[d = p_place_table[0][i]]) != 0) {
                if (postavy[j - 1].lives) {
                    set_font(H_FLITT5, barvy_skupin[postavy[j - 1].groupnum]);
                    draw_player(ablock(H_POSTAVY + j - 1), celx, cely,
                            p_positions_x[d], p_positions_y[d], HUMAN_ADJUST,
                            postavy[j - 1].jmeno);
                } else
                    draw_player(ablock(H_KOSTRA), celx, cely, p_positions_x[d],
                            p_positions_y[d] - 32, HUMAN_ADJUST, NULL);
            }


     }
  }



int draw_sloup_sector(int celx,int cely,int sector)
  {
  TSTENA *w,*q;
  int obl;

  int tmask = true_seeing?SD_TRUESEE:0;


  w=&map_sides[sector*4];
  q=&w[dirs[1]];
  obl=GET_OBLOUK(q);
  if (q->flags & SD_LEFT_ARC && q->oblouk)
     show_cel2(celx,cely,ablock(num_ofsets[OBL_NUM]+obl),0,0,1, ghost_walls);
  if (q->flags & SD_RIGHT_ARC && q->oblouk)
     show_cel2(celx,cely,ablock(num_ofsets[OBL2_NUM]+obl),0,0,2, ghost_walls);
  if (q->flags & (SD_PRIM_VIS|tmask) && q->prim  )
      show_cel2(celx,cely,ablock(num_ofsets[MAIN_NUM]+q->prim+(q->prim_anim>>4)),0,0,1+(q->oblouk & SD_POSITION), ghost_walls | (q->flags & tmask));
  if (celx<=0)
     {
     q=&w[dirs[0]];
     if (q->flags & (SD_PRIM_VIS|tmask) && q->prim)
      show_cel(-celx,cely,ablock(num_ofsets[LEFT_NUM]+q->prim+(q->prim_anim>>4)),0,0,2+(q->oblouk & SD_POSITION), ghost_walls| (q->flags & tmask));
     }
  if (celx>=0)
     {
     q=&w[dirs[2]];
     if (q->flags & (SD_PRIM_VIS|tmask) && q->prim)
      show_cel(celx,cely,ablock(num_ofsets[RIGHT_NUM]+q->prim+(q->prim_anim>>4)),0,0,3+(q->oblouk & SD_POSITION), ghost_walls | (q->flags & tmask));
     }
  q=&w[dirs[1]];
  if (q->flags & (SD_SEC_VIS|tmask) && q->sec && cely!=0) {
     if (q->flags & SD_SPEC)
      show_cel2(celx,cely-1,ablock(num_ofsets[MAIN_NUM]+q->sec+(q->sec_anim>>4)),0,0,2, ghost_walls| (q->flags & tmask));
     else
      show_cel2(celx,cely-1,ablock(num_ofsets[MAIN_NUM]+q->sec+(q->sec_anim>>4)),(q->xsec<<1)+celx*(viewport_geometry[0][0][cely].x-viewport_geometry[0][0][cely-1].x)/2,q->ysec<<1,0, ghost_walls| (q->flags & tmask));
  }
  return 0;
  }


void swap_truesee(int ss)
  {
  int i;
  for(i=0;i<4;i++)
     {
     TSTENA *s=map_sides+i+ss;

     if (s->flags & SD_TRUESEE) s->flags^=SD_PRIM_VIS | SD_SEC_VIS | SD_AUTOMAP;
     }
  }

void draw_sector(int celx,int cely,int s)
  {
//  int ss;
  //ss=s<<2;
//  if (true_seeing) swap_truesee(ss);
  switch (map_sectors[s].sector_type)
     {
     case S_SSMRT:
     case S_SLOUP:
     case S_TELEPORT:
          draw_sloup_sector(celx,cely,s);
          draw_placed_items_normal(celx,cely,s,viewdir);
          break;
     case S_LODKA:
          draw_basic_sector(celx,cely,s);
          if (cely==0) draw_placed_items_normal(celx,cely,s,viewdir);
          draw_lodku(celx,cely);
          break;
     default:
          draw_basic_sector(celx,cely,s);
          draw_placed_items_normal(celx,cely,s,viewdir);
          break;
     }
//  if (true_seeing) swap_truesee(ss);
  }

void back_clear(int celx,int color)
  {
  int x1,y1,x2,y2,xc;
  y1=viewport_geometry[0][0][VIEW3D_Z].y+MIDDLE_Y+SCREEN_OFFLINE;
  y2=viewport_geometry[0][1][VIEW3D_Z].y+MIDDLE_Y+SCREEN_OFFLINE;
  x2=viewport_geometry[0][1][VIEW3D_Z].x+MIDDLE_X;
  x1=-viewport_geometry[0][1][VIEW3D_Z].x+MIDDLE_X;
  xc=(x2-x1+2)*celx;
  x1=x1+xc-1;
  x2=x2+xc;
  if (x1<0) x1=0;
  if (x2>640) x2=640;
  if (x1<x2)
     {
     curcolor=color;
     RedirectScreenBufferSecond();
     bar32(x1,y1,x2,y2);
     RestoreScreen();
     }
  }

static char set_blind(void) {
    int i;

    if (battle && postavy[select_player].sektor == viewsector) {
        if (postavy[select_player].vlastnosti[VLS_KOUZLA] & SPL_BLIND)
            return 1;
        else
            return 0;
    }
    for (i = 0; i < POCET_POSTAV; i++) {
        THUMAN *p = postavy + i;
        if (p->sektor == viewsector && !(p->vlastnosti[VLS_KOUZLA] & SPL_BLIND))
            return 0;
    }
    return 1;
}

extern char folow_mode;

static void zobraz_lodku(const word *lodka, word *screen, int size)
  {
    int32_t scr_linelen2 = GetScreenPitch();

  lodka+=3;
  int x;
  while (size)
	{
	for (x=0;x<640 && size;x++)
	  {
	  if (*lodka!=0x8000) screen[x]=*lodka;
	  lodka++;
	  size--;
	  }
	screen+=scr_linelen2;
	}
  }
/*
static __inline void zobraz_lodku(word *data,word *scr,int _size)
  {
  __asm
    {
    mov ecx,_size
    mov esi,data
    mov edi,scr
    add esi,6
lp1:lodsw
    or   ax,ax
    jz   skp
    mov  [edi],ax
skp: add  edi,2h
    dec  ecx
    jnz  lp1
    }
  }
*/
/*
static void trace_for_bgr(int dir)
  {
	int s,i;
	static int olddist=0;
	static int olddir=0;
	TSTENA *ss;

	  int32_t scr_linelen2 = GetScreenPitch();


	bgr_handle=0;
	bgr_distance=-1;
	for(i=0,s=0;i<VIEW3D_Z && minimap[i][VIEW3D_X];)
	  {
	  s=minimap[i++][VIEW3D_X];
	  ss=map_sides+s*4+dir;
	  if (ss->flags & SD_PRIM_VIS && !ss->prim)
	  	{
			bgr_distance=i-1;
			bgr_handle=num_ofsets[BACK_NUM]+dir;
			break;
			}
		}
	if (olddist!=bgr_distance || olddir!=dir)
			{
			if (bgr_distance==-1)
				{
				word *w=GetScreenAdr();int i=scr_linelen2*480;
				do {if (*w>=NOSHADOW(0)) *w=back_color;w++;} while(--i);
				}
			zneplatnit_block(H_BGR_BUFF);
			}
	olddist=bgr_distance;
	olddir=dir;
	}
*/

void render_scene(int sector, int smer)
  {
  int i,j,s;

  see_monster=0;
  destroy_fly_map();
  build_fly_map();
  if (set_blind() && cur_mode!=MD_END_GAME && !folow_mode)
     {
     clear_buff(NULL,0,360);
     return;
     }
  if (set_halucination)
     {
     sector=hal_sector;
     smer=hal_dir;
     }
  cur_sector=sector;
  create_minimap(sector,smer);
//  trace_for_bgr(smer);
  i=VIEW3D_Z-1;
  s=minimap[i][VIEW3D_X];
  if (s && !map_sectors[s].ceil) clear_buff(ablock_copy(H_BGR_BUFF),back_color,360);
  else clear_buff(ablock_copy(H_BGR_BUFF),back_color,80);
  for(i=-VIEW3D_X+1;i<VIEW3D_X;i++)
     if ((s=minimap[VIEW3D_Z-1][VIEW3D_X+i])!=0)
        if (map_coord[s].flags & MC_SHADING) back_clear(i,0);
/*  if (reverse_draw)
  for(i=0;i<VIEW3D_Z;i++)
     for(j=0;j<VIEW3D_X;j++)
     {
     if (minimap[VIEW3D_X+j][i]) draw_sector(j,i,minimap[VIEW3D_X+j][i]);
     if (j && minimap[VIEW3D_X-j][i]) draw_sector(-j,i,minimap[VIEW3D_X-j][i]);
     do_events();
     if (cancel_render) return;
     }
  else*/
  for(i=VIEW3D_Z-1;i>=0;i--)
     {
        word *p;

        p=minimap[i];
        left_shiftup=0;right_shiftup=0;
        for(j=VIEW3D_X-1;j>=0;j--)
           {
           int s2;
           if ((s=p[VIEW3D_X+j])!=0)
              {
              if (map_coord[s].flags & MC_SHADING) secnd_shade=1; else secnd_shade=0;
              draw_basic_floor(j,i,s);
              if (map_sides[(s<<2)+smer].flags & SD_TRANSPARENT)
                {
                s2=map_sectors[s].step_next[smer];
                draw_mob(s2,smer,j,i,1);
                }
              draw_sector(j,i,s);
              }
           if (j && (s=p[VIEW3D_X-j]))
              {
              if (map_coord[s].flags & MC_SHADING) secnd_shade=1; else secnd_shade=0;
              draw_basic_floor(-j,i,s);
              if (map_sides[(s<<2)+smer].flags & SD_TRANSPARENT)
                {
                s2=map_sectors[s].step_next[smer];
                draw_mob(s2,smer,-j,i,1);
                }
              draw_sector(-j,i,s);
              }
           }
        for(j=VIEW3D_X-1;j>=0;j--)
           {
           if ((s=p[VIEW3D_X+j])!=0)
              {
              if (map_coord[s].flags & MC_SHADING) secnd_shade=1; else secnd_shade=0;
              draw_players(s,viewdir,j,i);
              draw_mob(s,viewdir,j,i,0);
              draw_fly_items(j,i,s,viewdir);
              draw_spectxtrs(s,j,i);
              }
           if (j && (s=p[VIEW3D_X-j]))
              {
              if (map_coord[s].flags & MC_SHADING) secnd_shade=1; else secnd_shade=0;
              draw_players(s,viewdir,-j,i);
              draw_mob(s,viewdir,-j,i,0);
              draw_fly_items(-j,i,s,viewdir);
              draw_spectxtrs(s,-j,i);
              }
           }
     }
  calc_spectxtrs();
  if (lodka) {
      if (!lodka_battle_draw && !running_anm) {
          if (!lodka_loaded) {
              game_display_load_sprite(H_LODKA, (const unsigned short *)ablock(H_LODKA));
              lodka_loaded = 1;
          }
          game_display_place_sprite(H_LODKA, 0, SCREEN_OFFLINE+301);
      } else {
          game_display_hide_sprite(H_LODKA);
          zobraz_lodku(ablock(H_LODKA),LODKA_POS,LODKA_SIZ);
          lodka_battle_draw = 0;
      }
  }
  }


void death_screen() {
    trans_bar(0, 0, 640, 480, 0);
    int xs;
    int ys;
    int y = 160;
    const char *t = texty[65];
    DECL_VLA(char, buff, strlen(t)+4);
    set_font(H_FBOLD, RGB555_ALPHA(31,31,31));
    zalamovani(t,buff, 440, &xs, &ys);
    t = buff;
    while (*t) {
        set_aligned_position(320, y, 1, 1, t);
        outtext(t);
        y+=2*text_height(t);
        t = t+strlen(t)+1;
    }


}


void redraw_scene()
  {
  if (norefresh) return;
  if (one_buffer) RedirectScreenBufferSecond();
  render_scene(viewsector,viewdir);
  if (running_anm) klicovani_anm(GetBuffer2nd()+SCREEN_OFFSET,anim_render_buffer,anim_mirror);
  update_mysky();
  schovej_mysku();
  if (one_buffer) RestoreScreen();
  OutBuffer2nd();
  if (battle || minimap_enabled || (game_extras & EX_ALWAYS_MINIMAP)) draw_medium_map();
  other_draw();
  if (cur_mode == MD_END_GAME) {
      death_screen();
  }
  ukaz_mysku();
  global_anim_counter++;
  send_message(E_KOUZLO_ANM);
  }

void refresh_scene(THE_TIMER *t)
  {
  redraw_scene();
  if (!norefresh)
     {
     showview(0,0,0,0);
     calc_game();
	 if (autoopenaction==1) {go_book(0,0,0,0,0);autoopenaction=0;}
     }
  }

typedef struct fx_play
  {
  int x,y,phase;
  struct fx_play *next;
  }FX_PLAY;

static FX_PLAY *fx_data=NULL;

void draw_fx()
  {
    const word *c;
  FX_PLAY *fx;
  FX_PLAY **last;

  int32_t scr_linelen2 = GetScreenPitch();
  if (fx_data==NULL) return;
  fx=fx_data;last=&fx_data;
  c=ablock(H_FX);
  while (fx!=NULL)
     {
     put_8bit_clipped(c,GetScreenAdr()+fx->x+fx->y*scr_linelen2,fx->phase*16,c[0],16);
     if (++fx->phase>14)
        {
        *last=NULL;
        free(fx);
        return;
        }
     last=&(fx->next);
     fx=*last;
     }
  }


void play_fx(int x,int y)
  {
  FX_PLAY *fx;

  fx=New(FX_PLAY);
  fx->next=fx_data;
  fx->x=x;
  fx->y=y;
  fx->phase=0;
  fx_data=fx;
  }

void play_fx_at(int where)
  {
  static word polex[]={313,290,362,336,165};
  static word poley[]={1,1,1,1,1,1};

  play_fx(polex[where],poley[where]);
  }

void display_ver(int x,int y,int ax,int ay)
  {
  char *ver="Br�ny Skeldalu version "VERSION" (C)1998";
  set_font(H_FTINY,RGB555(31,31,31));set_aligned_position(x,y,ax,ay,ver);
  outtext(ver);showview(0,0,0,0);
  }

void hide_boat() {
    game_display_hide_sprite(H_LODKA);
}
