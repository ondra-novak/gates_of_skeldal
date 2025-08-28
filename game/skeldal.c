#include <platform/platform.h>
#include <platform/achievements.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <libs/pcx.h>
#include <libs/bgraph.h>
#include <libs/event.h>
#include <libs/bmouse.h>
#include <libs/memman.h>
#include <platform/sound.h>
#include <libs/strlite.h>
#include <libs/gui.h>
#include <libs/basicobj.h>
#include <libs/mgfplay.h>
#include <libs/inicfg.h>
#include <platform/save_folder.h>
#include "globals.h"
#include "resources.h"
//
#include "advconfig.h"
#include "ach_events.h"
#include "skeldal.h"
#include "lang.h"

#include <ctype.h>
#define CONFIG_NAME SKELDALINI

#define INI_TEXT 1
#define INI_INT 2

#define ERR_GENERAL 1
const char *gpathtable[SR_COUNT];

/*
char *pathtable[]=
  {"",
  "graphics\\",
  "graphics\\basic\\",
  "graphics\\items\\",
  "samples\\",
  "font\\",
  "maps\\",
  "music\\",
  "",
  "graphics\\enemies\\",
  "video\\",
  "graphics\\dialogs\\"
  };
*/

char **texty;

char skip_intro=0;
char autosave_enabled=1;
int32_t game_time=0;
int charmin=3;
int charmax=3;

int autoopenaction=0;
int autoopendata=0;


void redraw_desktop_call(void);

TMA_LOADLEV loadlevel;

typedef struct inis
  {
  char heslo[50];
  char parmtype;
  }INIS;

THE_TIMER timer_tree;



int hl_ptr=H_FIRST_FREE;
int debug_enabled=0;
char sound_detection=1;
int snd_devnum,snd_parm1,snd_parm2,snd_parm3,snd_mixing=22000;
uint8_t gamespeed=GAMESPEED;
uint8_t gamespeedbattle=GAMESPEED;
char level_preload=1;
char *level_fname=NULL;
int game_extras=0;

char default_map[20]="LESPRED.MAP";

THUMAN postavy[POCET_POSTAV],postavy_save[POCET_POSTAV];
void (*unwire_proc)(void);
void (*wire_proc)(void);
char cur_mode,battle_mode;


const void *pcx_fade_decomp(const void *p, int32_t *s, int h);
const void *pcx_15bit_decomp(const void *p, int32_t *s, int h);
const void *pcx_15bit_decomp_transp0(const void *p, int32_t *s, int h);
const void *pcx_15bit_autofade(const void *p, int32_t *s, int h);
const void *pcx_15bit_backgrnd(const void *p, int32_t *s, int h);
const void *pcx_8bit_decomp(const void *p, int32_t *s, int h);
const void *pcx_fade_decomp(const void *p, int32_t *s, int h);
const void *load_text_decomp(const void *p, int32_t *s, int h);

const char *texty_knihy;
static const char *patch_file=NULL;
int cur_page=0;

TSTR_LIST cur_config=NULL;

TDREGISTERS registred[]=
  {
    {H_DESK,"desk.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_TOPBAR,"topbar.pcx",pcx_15bit_decomp,SR_BGRAFIKA},
    {H_OKNO,"okno.pcx",pcx_15bit_decomp,SR_BGRAFIKA},
    {H_MS_DEFAULT,"msc_sip.pcx", pcx_8bit_decomp,SR_BGRAFIKA},
    {H_MS_SOUBOJ,"msc_x.pcx", pcx_8bit_decomp,SR_BGRAFIKA},
    {H_MS_WHO,"msc_who.pcx", pcx_8bit_decomp,SR_BGRAFIKA},
//    {H_MS_LIST,"msc_list.pcx", pcx_8bit_decomp,SR_BGRAFIKA},
    {H_MS_ZARE,"msc_zare.pcx", pcx_8bit_decomp,SR_BGRAFIKA},
    {H_KOMPAS,"kompas.pcx", pcx_15bit_decomp,SR_BGRAFIKA},
    {H_SIPKY_S,"sipky_s.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SIPKY_SV,"sipky_sv.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SIPKY_SZ,"sipky_sz.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SIPKY_V,"sipky_v.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SIPKY_Z,"sipky_z.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SIPKY_J,"sipky_j.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_BACKMAP,"backmap.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_FBOLD,"sada16.fon",NULL,SR_FONT},
    {H_FSYMB,"ikones.fon",NULL,SR_FONT},
    {H_FLITT,"font4x8.fon",NULL,SR_FONT},
    {H_FLITT5,"font5x8.fon",NULL,SR_FONT},
    {H_FONT6,"font6x9.fon",NULL,SR_FONT},
    {H_FONT7,"sada7.fon",NULL,SR_FONT},
    {H_FTINY,"tiny.fon",NULL,SR_FONT},
    {H_FKNIHA,"kniha.fon",NULL,SR_FONT},
    {H_FBIG,"timese.fon",NULL,SR_FONT},
    {H_IOBLOUK,"ioblouk.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_LODKA,"lodka.pcx",pcx_15bit_decomp_transp0,SR_BGRAFIKA},
    {H_IDESKA,"ideska.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_IMRIZ1,"imriz1.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_RAMECEK,"ramecek.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_ENEMY,"enemy.dat",load_mob_legacy_format,SR_MAP},
    {H_BATTLE_BAR,"souboje.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_BATTLE_MASK,"m_souboj.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_MZASAH1,"mzasah1.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_MZASAH2,"mzasah2.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_MZASAH3,"mzasah3.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_PZASAH,"pzasah.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_BATTLE_CLICK,"souboje2.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SIPKY_END,"sipky.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_KOUZLA,"kouzla.dat",load_spells_legacy_format,SR_MAP},
    {H_LEBKA,"death.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_KOSTRA,"bones.pcx",pcx_fade_decomp,SR_BGRAFIKA},
    {H_RUNEHOLE,"runehole.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_RUNEMASK,"runemask.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_POWERBAR,"powerbar.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_POWERLED,"powerled.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_POSTAVY_DAT,"postavy.dat",NULL,SR_MAP},
    {H_SOUND_DAT,"sound.dat",NULL,SR_MAP},
    {H_SND_SWHIT1,"swd_hit0.wav",wav_load,SR_ZVUKY},
    {H_SND_SWHIT2,"swd_hit1.wav",wav_load,SR_ZVUKY},
    {H_SND_SWMISS1,"swd_mis0.wav",wav_load,SR_ZVUKY},
    {H_SND_SWMISS2,"swd_mis1.wav",wav_load,SR_ZVUKY},
    {H_SND_SIP1,"sip2.wav",wav_load,SR_ZVUKY},
    {H_SND_SIP2,"sip1.wav",wav_load,SR_ZVUKY},
    {H_SND_KNIHA,"kniha.wav",wav_load,SR_ZVUKY},
    {H_SND_OBCHOD,"obchod.wav",wav_load,SR_ZVUKY},
    {H_SND_LEKTVAR,"lektvar.wav",wav_load,SR_ZVUKY},
    {H_SND_TELEPIN,"telepin.wav",wav_load,SR_ZVUKY},
    {H_SND_TELEPOUT,"telepout.wav",wav_load,SR_ZVUKY},
    {H_SND_HEK1M,"jauu1m.wav",wav_load,SR_ZVUKY},
    {H_SND_HEK2M,"jauu2m.wav",wav_load,SR_ZVUKY},
    {H_SND_HEK1F,"jauu1f.wav",wav_load,SR_ZVUKY},
    {H_SND_HEK2F,"jauu2f.wav",wav_load,SR_ZVUKY},
    {H_SND_EAT,"jidlo.wav",wav_load,SR_ZVUKY},
    {H_SND_WEAR,"obleci.wav",wav_load,SR_ZVUKY},
    {H_SND_PUTINV,"put_inv.wav",wav_load,SR_ZVUKY},
    {H_RUNEBAR1,"r_ohen.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_RUNEBAR2,"r_voda.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_RUNEBAR3,"r_zeme.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_RUNEBAR4,"r_vzduch.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_RUNEBAR5,"r_mysl.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SPELLDEF,"spelldef.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_KNIHA,"kniha.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_WINTXTR,"wintxtr.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SAVELOAD,"saveload.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SVITEK,"svitek.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_LOADTXTR,"loadtxtr.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_DIALOG,"dialog.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_DIALOGY_DAT,"dialogy.dat",NULL,SR_MAP},
    {H_SHOP_PIC,"shop.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_TELEPORT,"teleport.mgf",NULL,SR_BGRAFIKA},
    {H_FX,"fx.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_CHECKBOX,"checkbox.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SETUPBAR,"volbades.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SOUPAK,"volbasou.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_SETUPOK,"volbazpe.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_POSTUP,"postup.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_LODKA0,"lesda21a.pcx",pcx_fade_decomp,SR_GRAFIKA},
    {H_LODKA1,"lesda22a.pcx",pcx_fade_decomp,SR_GRAFIKA},
    {H_LODKA2,"lesda23a.pcx",pcx_fade_decomp,SR_GRAFIKA},
    {H_LODKA3,"lesda24a.pcx",pcx_fade_decomp,SR_GRAFIKA},
    {H_LODKA4,"lesda25a.pcx",pcx_fade_decomp,SR_GRAFIKA},
    {H_LODKA5,"lesda26a.pcx",pcx_fade_decomp,SR_GRAFIKA},
    {H_LODKA6,"lesda27a.pcx",pcx_fade_decomp,SR_GRAFIKA},
    {H_LODKA7,"lesda28a.pcx",pcx_fade_decomp,SR_GRAFIKA},
    {H_FLETNA,"fletna.wav",wav_load,SR_ZVUKY},
    {H_FLETNA_BAR,"stupnice.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_FLETNA_MASK,"stupni_m.pcx",pcx_8bit_nopal,SR_BGRAFIKA},
    {H_SND_SEVER,"sever.wav",wav_load,SR_ZVUKY},
    {H_SND_VYCHOD,"vychod.wav",wav_load,SR_ZVUKY},
    {H_SND_JIH,"jih.wav",wav_load,SR_ZVUKY},
    {H_SND_ZAPAD,"zapad.wav",wav_load,SR_ZVUKY},
    {H_SND_RAND1,"random1.wav",wav_load,SR_ZVUKY},
    {H_SND_RAND2,"random2.wav",wav_load,SR_ZVUKY},
    {H_SND_RAND3,"random3.wav",wav_load,SR_ZVUKY},
    {H_SND_RAND4,"random4.wav",wav_load,SR_ZVUKY},
    {H_CHARGEN,"chargen.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_CHARGENB,"chargenb.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_CHARGENM,"chargenm.pcx",pcx_8bit_nopal,SR_BGRAFIKA},
    {H_BGR_BUFF,"",set_background,SR_BGRAFIKA},
    {H_KREVMIN,"krevmin.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_KREVMID,"krevmid.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_KREVMAX,"krevmax.pcx",pcx_8bit_decomp,SR_BGRAFIKA},
    {H_GLOBMAP,"globmap.dat",load_text_decomp, SR_MAP}
	};

INIS sinit[]=
  {
  {"VMODE",INI_INT},
  {"ZOOM_SPEED",INI_INT},
  {"TURN_SPEED",INI_INT},
  {"MUSIC_VOLUME",INI_INT},
  {"SOUND_VOLUME",INI_INT},
  {"SOUND_DEVICE",INI_TEXT},
  {"SOUND_MIXFREQ",INI_INT},
  {"DEFAULT_MAP",INI_TEXT},
  {"GAME_SPEED",INI_INT},
  {"PRELOAD",INI_INT},
  {"INSTALL",INI_TEXT},
  {"PATCH",INI_INT},
  {"SKIP_INTRO",INI_INT},
  {"AUTOSAVE",INI_INT},
  {"DEBUG",INI_INT},
  {"FULLRESVIDEO",INI_INT},
  {"PATCH_FILE",INI_TEXT},
  {"TITLES",INI_INT},
  {"CHAR_MIN",INI_INT},
  {"CHAR_MAX",INI_INT},
  {"EXTRAS",INI_INT},
  {"WINDOWED", INI_INT},
  {"BATTLE_ACCEL",INI_INT},
  {"WINDOWEDZOOM", INI_INT},
  {"MONITOR",INI_INT},
  {"VERSION",INI_INT},
  {"REFRESHRATE",INI_INT},
  {"CESTA_DATA",INI_TEXT},
  {"CESTA_GRAFIKA",INI_TEXT},
  {"CESTA_ZVUKY",INI_TEXT},
  {"CESTA_FONTY",INI_TEXT},
  {"CESTA_MAPY",INI_TEXT},
  {"CESTA_MUSIC",INI_TEXT},
  {"CESTA_TEMPY",INI_TEXT},
  {"CESTA_BGRAFIKA",INI_TEXT},
  {"CESTA_ITEMY",INI_TEXT},
  {"CESTA_ENEMY",INI_TEXT},
  {"CESTA_VIDEO",INI_TEXT},
  {"CESTA_DIALOGY",INI_TEXT},
  {"CESTA_POZICE",INI_TEXT},
  {"CESTA_HLAVNI",INI_TEXT},
  {"CESTA_CD",INI_TEXT},
  {"CESTA_MAPY2",INI_TEXT},
  {"CESTA_PLUGINS",INI_TEXT},
  {"CESTA_ORIG_MUSIC",INI_TEXT}
  };


#define CESTY_POS 27
int last_ms_cursor=-1;
int vmode=2;


void purge_temps(char _) {
    temp_storage_clear();
}

const void *pcx_fade_decomp(const void *p, int32_t *s, int h)
  {
  char *buff;
  int r = load_pcx(p,*s,A_FADE_PAL,&buff,mglob.fade_r,mglob.fade_g,mglob.fade_b);
  assert(r > 0);
  *s=r;
  return buff;
  }

const void *pcx_15bit_decomp(const void *p, int32_t *s, int h)
  {
  char *buff;
  int r = load_pcx(p,*s,A_16BIT,&buff);
  assert(r > 0);
  *s=r;
  return buff;
  }
const void *pcx_15bit_decomp_transp0(const void *p, int32_t *s, int h)
  {
  char *buff;
  int r = load_pcx(p,*s,A_16BIT_ZERO_TRANSP,&buff);
  assert(r > 0);
  *s=r;
  return buff;
  }

const void *pcx_15bit_autofade(const void *p, int32_t *s, int h)
  {
  char *buff;
  int r = load_pcx(p,*s,A_16BIT,&buff);
  assert(r > 0);
  *s=r;
  buff[5]=0x80;
  return buff;
  }

const void *pcx_15bit_backgrnd(const void *p, int32_t *s, int h)
  {
  char *buff;
  int32_t i;int32_t *z;

  if (p!=NULL)
     {
     int r = load_pcx(p,*s,A_16BIT,&buff);
     assert(r>0);
     z=(int32_t *)buff;
     *s=r;
     for(i=*s;i>0;i-=4,z++) *z|=0x80008000;
     return buff;
     }
  return NULL;
  }

const void *pcx_8bit_nopal(const void *p,int32_t *s, int h)
  {
  char *buff = NULL;

  if (p!=NULL)
     {
     int r = load_pcx(p,*s,A_8BIT_NOPAL,&buff);
     assert(r>0);
     *s=r;
     }
  return buff;
  }



const void *pcx_8bit_decomp(const void *p, int32_t *s, int h)
  {
  char *buff;
  int r = load_pcx(p,*s,A_8BIT,&buff);
  assert(r>0);
  *s=r;
  return buff;
  }

const void *hi_8bit_correct(const void *p,int32_t *s, int h)
{
    word *out = (word *)getmem(*s);
    memcpy(out, p, *s);
  word *ptr=out;
  int i;
  if (ptr[2]==8)
  {
	for (i=0;i<256;i++)
	{
	  ptr[i+3]=((ptr[i+3] & ~0x1F)+ptr[i+3]);
	}
  }
  return out;
}

const void *load_text_decomp(const void *p, int32_t *s, int h) {
    char *newbuff = malloc(*s+1);
    memcpy(newbuff,p, *s);
    newbuff[*s] = 0;
    (*s)+=1;
    return newbuff;
}

const void *load_mob_legacy_format_direct(const void *p, int32_t *s, int h) {
    const char *c = p;
    const int sz = 376;
    int count = *s / sz;;
    TMOB *out = getmem(count * sizeof(TMOB));

    memset(out, 0 , sizeof(TMOB)*count);
    for (int i = 0; i < count ; i++) {
        TMOB *m = out+i;
        char *d = (char *)m;
        size_t ofs = 0;
        size_t nx = offsetof(TMOB,experience);
        memmove(d, c, nx);
        c+=nx-2; //first padding 2
        d+=nx;
        ofs=nx;
        nx = offsetof(TMOB, dialog_flags);
        memmove(d, c, nx - ofs);
        c+=nx - ofs -1; //second padding 1
        d+=nx-ofs;
        ofs=nx;
        nx = sizeof(TMOB);
        memmove(d, c, nx - ofs - 1); //last padding 1
        c+=nx - ofs - 1;
        m->vlajky2 = 0;
    }
    *s = count * sizeof(TMOB);
    return out;
}
const void *load_mob_legacy_format(const void *p, int32_t *s, int h) {
    const char *c = p;
    c+=8;
    *s-=8;
    return load_mob_legacy_format_direct(c, s,h);
}


const void *set_background(const void *p, int32_t *s, int h)
  {
  const word *data;
  word *ptr;
  const word *pal;
  char *pic;
  void *out;
  int counter;


  if (!bgr_handle) return p;
  if (bgr_distance==-1) return p;
  int32_t scr_linelen2 = GetScreenPitch();
  data=ablock(bgr_handle);
  *s=scr_linelen2*360*2;
  out = ptr=getmem(*s);
  counter=scr_linelen2*360;
  pal=data+3+bgr_distance*256;
  pic=(char *)data+PIC_FADE_PAL_SIZE;
  do
	*ptr++=pal[(uint8_t)*pic++] | BGSWITCHBIT;
  while (--counter);
  return out;
  }

void mouse_set_cursor(int cursor)
  {
  static short *ms_item=NULL;

  if (cursor==last_ms_cursor) return;
  if (last_ms_cursor>0) aunlock(last_ms_cursor);
  if (cursor>0)
     {
     alock(cursor);
     schovej_mysku();
     nastav_mysku_kurzor(ablock(cursor),0,0);
     last_ms_cursor=cursor;
     ukaz_mysku();
     }
  else
     {
     char *p;

     cursor=-cursor;
     if (ms_item==NULL) ms_item=(short *)getmem(IT_ICONE_SIZE);
     p=(char *)ablock(cursor/18+ikon_libs);
     memcpy(ms_item,&p[(cursor%18)*IT_ICONE_SIZE],IT_ICONE_SIZE);
     schovej_mysku();
     nastav_mysku_kurzor(ms_item,45/2,55/2);

     last_ms_cursor=-cursor;
     ukaz_mysku();
     }
  }

void mouse_set_default(int cursor)
  {
  default_ms_cursor=cursor;
  mouse_set_cursor(cursor);
  }

void set_font(int font,int c1,...)
  {
  static int last_font=-1;
  va_list lst;
  va_start(lst, c1);
  int i;

  if (last_font!=-1 && last_font!=font)
     {
     aunlock(last_font);
     alock(font);
     }
  curfont=ablock(font);
  if (c1>=0)
     if (c1 & BGSWITCHBIT)
      {
      charcolors[0]=0xFFFF;
      for (i=1;i<5;i++) charcolors[i]=c1 & ~BGSWITCHBIT;
      }
     else if (c1 & FONT_TSHADOW) {
         if ((c1 & FONT_TSHADOW_GRAY) == FONT_TSHADOW_GRAY)
             charcolors[0]=RGB555_ALPHA(16,16,15);
         else
             charcolors[0]=BGSWITCHBIT;
         for (i=1;i<5;i++) charcolors[i]=c1 & ~0x20;
     }else
      {
      charcolors[0]=0;
      for (i=1;i<5;i++) charcolors[i]=c1 & ~0x20;
      }
  else if (c1==-2)
     {
     for (i=0;i<7;i++) charcolors[i]=va_arg(lst, int);
     }
  last_font=font;
  default_font=curfont;
  memcpy(f_default,charcolors,sizeof(charcolors));
  }


void clrscr(void)
  {

  }





void back_music(THE_TIMER *t)
  {
  mix_back_sound(0);
  }

/*void *anim_idle(EVENT_MSG *msg,void **usr)
  {
  usr;
  if (msg->msg==E_TIMER) calc_animations();
  return &anim_idle;
  }*/

/*void timer_error(void)
  {
  puts("\x7");
  }
*/
void timming(EVENT_MSG *msg,void **data)
  {
  THE_TIMER *p,*q;
  int i,j;

  data;
  if (msg->msg==E_INIT) return ;
  *otevri_zavoru=1;
  j=va_arg(msg->data,int);
  for (i=0;i<j;i++)
  {
  p=&timer_tree;
  q=p->next;
  while (q!=NULL)
     {
     p=q->next;
//     if (p!=NULL && p->zero!=0) timer_error();
     if (!(--q->counter)) {
     if (q->zavora && i==(j-1))
        {
        q->zavora=0;
        if (q->calls!=-2) q->proc(q);
        p=q->next;
        q->zavora=1;
        q->counter=q->count_max;
        if (q->calls!=-1) {
           if (--q->calls<1)
              {
              for(p=&timer_tree;p->next!=q;p=p->next);
              p->next=q->next;
              #ifdef LOGFILE
              if (q->next==NULL)  {

              } else {
                 SEND_LOG("(TIMER) Self remove for timer id: %d, next->%d",q->id,q->next->id);
              }
              #endif
              free(q);
              q=p;
              }
           //else
             // q->counter=1;
        }
        }
     else {
        q->counter=1;
     }
  }
     if (q->next!=p && q!=p)
        {
        THE_TIMER *z;

        z=&timer_tree;while(z->next!=p && z->next!=NULL) z=z->next;
        if (z->next==NULL) return;
        }
     q=p;
     }
  }
  return ;
  }

void delete_from_timer(int id)
  {
  THE_TIMER *p,*q;

  p=&timer_tree;
  q=p->next;
  while (q!=NULL)
     {
     if (q->id==id)
              {
              if (q->zavora)
                 {
                 #ifdef LOGFILE
                 if (q->next==NULL) {

                 }else {
                    SEND_LOG("(TIMER) Removing timer id: %d, next->%d",id,q->next->id);
                 }
                 #endif
                 p->next=q->next;
                 free(q);
                 q=p;
                 }
              else
                 {

                 q->calls=-2;
                 q->counter=1;
                 }
              }
      p=q;q=q->next;
     }
  }

THE_TIMER *find_timer(int id)
  {
  THE_TIMER *p;

  p=timer_tree.next;
  while (p!=NULL && p->id!=id) p=p->next;
  return p;
  }


void hold_timer(int id,char hld)
  {
  THE_TIMER *q;

  q=timer_tree.next;
  while (q!=NULL && q->id!=id) q=q->next;
  if (q!=NULL) q->counter=1-(hld<<1);
  SEND_LOG("(TIMER) Timer hold id: %d status: %s",id,hld?"Hold":"Unhold");
  }

THE_TIMER *add_to_timer(int id,int delay,int maxcall,TIMER_PROC proc)
  {
  THE_TIMER *q;

//  if (id==2 && marker && timer_tree.next->id==2)
//     MARKER_HIT(timer_error());
  q=(THE_TIMER *)getmem(sizeof(THE_TIMER));
  q->counter=q->count_max=delay;
  q->calls=maxcall;
  q->proc=proc;
  q->id=id;
  q->next=timer_tree.next;
  q->zavora=1;
  q->zero=0;
  timer_tree.next=q;
  SEND_LOG("(TIMER) Adding to timer id: %d delay: %d",id,delay);
  return q;
  }

static void kill_timer(void)
  {
  THE_TIMER *t;

  t=timer_tree.next;
  while (t!=NULL)
     {
     THE_TIMER *p;

     p=t;t=t->next;free(p);
     }
  timer_tree.next=NULL;
  }

void user_timer(EVENT_MSG *msg,void **usr)
  {
  int x;
  static int lastvalue=0;
  usr;
  if (msg->msg==E_WATCH)
     {
     *otevri_zavoru=1;
     x=get_timer_value();
     x-=lastvalue;
     lastvalue+=x;
     x = MIN(TIMERSPEED, x);  //prevent clock skew
     if (x) send_message(E_TIMER,x);
     }
  }

void do_timer(void)
  {
  EVENT_MSG msg;

  msg.msg=E_IDLE;
  *otevri_zavoru=1;
  user_timer(&msg,NULL);
  }

void done_skeldal(void)
  {
  steam_shutdown();

  close_manager();
  close_story_file();
  temp_storage_clear();
  stop_mixing();
//  deinstall_mouse_handler();
    if (texty != NULL) {
        release_list(texty);
        texty = NULL;
    }
    if (cur_config != NULL) {
        release_list(cur_config);
        cur_config = NULL;
    }
  kill_timer();

  }


int cislovka(int i)
  {
  if (i==1) return 0;
  if (i>1 && i<5) return 1;
  return 2;
  }

void register_basic_data(void)
  {
  int i,s;
  TDREGISTERS *p;
  char xname[16];

  s=sizeof(registred)/sizeof(TDREGISTERS);
  p=registred;
  for(i=0;i<s;i++,p++) def_handle(p->h_num,p->name,p->proc,p->path);
  def_handle(H_BOTTBAR,"",bott_draw_proc,0);
  for(i=0;i<H_TELEP_CNT;i++)
     {
     sprintf(xname,"TELEP%02d.PCX",i);
     def_handle(H_TELEP_PCX+i,xname,pcx_fade_decomp,SR_BGRAFIKA);
     }
  for(i=0;i<H_ARMA_CNT;i++)
     {
     sprintf(xname,"ARMA%02d.PCX",i);
     def_handle(H_ARMAGED+i,xname,pcx_fade_decomp,SR_BGRAFIKA);
     }
  for(i=0;i<H_KILL_MAX;i++)
     {
     sprintf(xname,"KILL%02d.PCX",i);
     def_handle(H_KILL+i,xname,pcx_fade_decomp,SR_BGRAFIKA);
     }
  }

void reg_grafiku_postav(void)
  {
  int i;
  char xname[16];

  for(i=0;i<POCET_POSTAV;i++)
     {
     undef_handle(i+H_POSTAVY);
     undef_handle(i+H_XICHTY);
     undef_handle(i+H_CHARS);
     }
  for(i=0;i<POCET_POSTAV;i++) def_handle(i+H_POSTAVY,"",build_items_called,0);
  for(i=0;i<POCET_POSTAV;i++)
     {
     sprintf(xname,XICHT_NAME,postavy[i].xicht);
     def_handle(i+H_XICHTY,xname,pcx_8bit_decomp,SR_BGRAFIKA);
     sprintf(xname,CHAR_NAME,postavy[i].xicht);
     def_handle(i+H_CHARS,xname,pcx_fade_decomp,SR_BGRAFIKA);
     }
  }

void cti_texty(void)
  {
  int err;
  texty=(TSTR_LIST)create_list(4);
  const char *path = build_pathname(2,gpathtable[SR_DATA], TEXTY);
  if ((err=load_string_list_ex(&texty,path))!=0)
     {
	 char buff[256];

     switch (err)
        {
        case -1:sprintf(buff,"Can't load string table. File %s has not been found\n",path);break;
        case -2:sprintf(buff,"Missing end mark (-1) at the end of string table\n");break;
        case -3:sprintf(buff,"Memory very low (need min 4MB)\n");break;
        default:sprintf(buff,"Error in string table at line %d\n",err);break;
        }
    display_error(buff);
     exit(1);
     }

     //patch stringtable
     if (!texty[98]) str_replace(&texty,98,"Ulo\x91it hru jako");
     if (!texty[99]) str_replace(&texty,99,"CRT Filter (>720p)");
     str_replace(&texty, 144, "Zrychlit souboje");
     str_replace(&texty, 51, "Celkov\x88 Hudba Efekty  V\x98\xA8ky  Basy Rychlost");
     str_replace(&texty,0,"Byl nalezen p\xA9ipojen\x98 ovlada\x87\nPro aktivaci ovlada\x87""e stiskn\x88te kter\x82koliv tla\x87\xA1tko na ovlada\x87i");
     lang_patch_stringtable(&texty, "ui.csv", "");
  }




void global_kbd(EVENT_MSG *msg,void **usr)
  {
  char c;

  usr;
  if (msg->msg==E_KEYBOARD)
     {
     c=va_arg(msg->data,int)>>8;
     int32_t scr_linelen2 = GetScreenPitch();
     if (c==';') save_dump(GetScreenAdr(), DxGetResX(), DxGetResY(), scr_linelen2);
     }
  return;
  }

void  add_game_window(void)
  {
  WINDOW *p;
  CTL3D *c;

  c=def_border(0,0);
  p=create_window(0,0,0,0,0,c);
  desktop_add_window(p);
  }



void error_exception(EVENT_MSG *msg,void **unused)
  {
  if (msg->msg==E_PRGERROR)
     {
     unused;



     SEND_LOG("(ERROR) Log: Sector %d Direction %d",viewsector,viewdir);

     SEND_LOG("(ERROR) Log: Battle: %d Select_player %d",battle,select_player);

     display_error("error_exception called");

     }
  }



const void *boldcz;

#define ERR_WINX 320
#define ERR_WINY 100

/*

char device_error(int chyba,char disk,char info)
  {
  char c;
  void *old;

  old=_STACKLOW;
  _STACKLOW=NULL;
  chyba,disk,info;
  curfont=&boldcz;
  charcolors[0]=0xffff;
  for(c=1;c<5;c++) charcolors[c]=0x7fff;
  memcpy(buffer_2nd,screen,screen_buffer_size);
  trans_bar(320-ERR_WINX/2,240-ERR_WINY/2,ERR_WINX,ERR_WINY,0);
  curcolor=0x7fff;
  rectangle(320-ERR_WINX/2,240-ERR_WINY/2,320+ERR_WINX/2,240+ERR_WINY/2,0x7fff);
  set_aligned_position(320,230,1,1,texty[8]);outtext(texty[8]);
  set_aligned_position(320,250,1,1,texty[9]);outtext(texty[9]);
  showview(0,0,0,0);
  do
     {
     c=getche();
     }
  while (c!=13 && c!=27);
  memcpy(screen,buffer_2nd,screen_buffer_size);
  showview(0,0,0,0);
  _STACKLOW=old;
  return (c==13?_ERR_RETRY:_ERR_FAIL);
  }
*/

void init_DDL_manager() {

    const char *ddlfile = build_pathname(2, gpathtable[SR_DATA],"SKELDAL.DDL");
    ddlfile = local_strdup(ddlfile);

    init_manager();
    if (patch_file && !add_patch_file(patch_file)) {
        display_error("Can't open resource file (adv_patch): %s", ddlfile);
        abort();
    }
    const char *lang_fld = lang_get_folder();
    if (lang_fld) {
        const char *gfx = build_pathname(2, lang_fld, "gfx.ddl");
        gfx = local_strdup(gfx);
        add_patch_file(gfx);
    }
    if (!add_patch_file(ddlfile)) {
        display_error("Can't open resource file (main): %s", ddlfile);
        abort();
    }

    SEND_LOG("(GAME) Memory manager initialized. Using DDL: '%s'",ddlfile);

    register_basic_data();

}

void show_joystick_info(void) {


  curcolor = 0;
  set_font(H_FBOLD, RGB888(255,255,255));
  const char *prompt = texty[0];
  char *buff = (char *)alloca(strlen(prompt)+10);
  int xs = 0;
  int ys = 0;
  zalamovani(prompt,buff,560, &xs, &ys);
  int y = 100;
  while (*buff) {
    set_aligned_position(320,y,1,1,buff);
    outtext(buff);
    buff += strlen(buff)+1;
    y = y + 20;
  }

  showview(0,0,0,0);
  for (int i = 0; i < 100; ++i) {
    sleep_ms(100);
    if (is_joystick_used()) break;
    if (_bios_keybrd(_KEYBRD_READY)) {
      _bios_keybrd(_KEYBRD_READ);
      break;
    }
  }

}

void show_loading_picture(char *filename)
  {
  const void *p;
  int32_t s;

  p=afile(filename,SR_BGRAFIKA,&s);
  put_picture(0,0,p);
  showview(0,0,0,0);
  ablock_free(p);
  }

typedef int (*GAME_THREAD_CB)(va_list);

int init_skeldal_thread(va_list args) {

    const INI_CONFIG *cfg = va_arg(args, const INI_CONFIG *);
    GAME_THREAD_CB game_thread = va_arg(args,GAME_THREAD_CB);
    va_list *game_args = va_arg(args, va_list *);

    showview = game_display_update_rect;
    game_display_set_icon(getWindowIcon(), getWindowIconSize());
    init_joystick(ini_section_open(cfg, "controller"));

    general_engine_init();
    atexit(done_skeldal);

    init_DDL_manager();
    show_loading_picture("LOADING.HI");

    if (lang_get_folder()) {
        texty_knihy = build_pathname(2, lang_get_folder(), "book.txt");
        if (!check_file_exists(texty_knihy)) {
            texty_knihy=strdup(build_pathname(2,gpathtable[SR_MAP],"kniha.txt"));
        } else {
            texty_knihy=strdup(texty_knihy);
        }
    } else {
        texty_knihy=strdup(build_pathname(2,gpathtable[SR_MAP],"kniha.txt"));
    }

    install_gui();
    if (is_joystick_enabled()) {
      show_joystick_info();
    }



    send_message(E_DONE,E_WATCH,timer);
    send_message(E_DONE,E_IDLE,redraw_desktop_call);
    send_message(E_ADD,E_TIMER,timming);

    send_message(E_ADD,E_WATCH,user_timer);

    send_message(E_ADD,E_MOUSE,ms_clicker);

    send_message(E_ADD,E_KEYBOARD,global_kbd);

    send_message(E_ADD,E_PRGERROR,error_exception);

    add_to_timer(TM_BACK_MUSIC,5,-1,back_music);

    add_game_window();

    game_sound_init_device(ini_section_open(cfg, "audio"));
    start_mixing();

    int verr;
    if ((verr=init_mysky())!=0)
       {

       puts(texty[174-verr]);
       SEND_LOG("(ERROR) %s (%d)",texty[174-verr],verr);

       exit(0);
       }

  //  hranice_mysky(0,0,639,479);

    mouse_set_default(H_MS_DEFAULT);
    ukaz_mysku();
    set_end_of_song_callback(end_of_song_callback, NULL);

    kouzla_init();

    load_items();

    load_shops();
    memset(&loadlevel,0,sizeof(loadlevel));
    loadlevel.eflags = 0xFF;

    int r = game_thread(*game_args);
    va_end(args);
    return r;

}


int init_skeldal(const INI_CONFIG *cfg, void (*game_thread)(va_list), ...)
  {
  boldcz=LoadDefaultFont();

  cti_texty();
  timer_tree.next=NULL;
  init_events();

  steam_init();
  va_list args;
  va_start(args,game_thread);

  int verr = game_display_init(ini_section_open(cfg, "video"), "Skeldal",
              init_skeldal_thread, cfg, game_thread, &args);
  if (verr < 0)
     {
      display_error("Error game_display_init %d", verr);
      return 1;
     }
  return verr;
  }


void wire_main_functs();
void unwire_main_functs(void)
  {

  delete_from_timer(TM_FLY);
  delete_from_timer(TM_SCENE);
  delete_from_timer(TM_REGEN);
  send_message(E_DONE,E_KEYBOARD,game_keyboard);
  send_message(E_DONE,E_KROK,real_krok);
  console_show(0);
  disable_click_map();
  wire_proc=wire_main_functs;
  hide_boat();
  }


void wire_main_functs(void)
  {

  add_to_timer(TM_SCENE,gamespeed,-1,refresh_scene);
  add_to_timer(TM_FLY,gamespeed,-1,calc_fly);
  add_to_timer(TM_REGEN,500,-1,real_regeneration);
  send_message(E_ADD,E_KEYBOARD,game_keyboard);
  send_message(E_ADD,E_KROK,real_krok);
  change_click_map(clk_main_view,CLK_MAIN_VIEW);
  unwire_proc=unwire_main_functs;
  cur_mode=MD_GAME;
  running_battle=0;
  recalc_volumes(viewsector,viewdir);
  cancel_pass=1;
  }


void init_game(void)
  {

  init_inventory();

  reg_grafiku_postav();
  build_all_players();
  }

void *map_keyboard(EVENT_MSG *msg,void **usr);

char doNotLoadMapState=0;

static int reload_map_handler(EVENT_MSG *msg,void **usr)
{
extern char running_battle;
  if (msg->msg==E_RELOADMAP)
  {
	int i;
	ReloadMapInfo *minfo=va_arg(msg->data, ReloadMapInfo *);
	const char *fname=minfo->fname;
	int sektor=minfo->sektor;
	strcopy_n(loadlevel.name,fname,sizeof(loadlevel.name));
	loadlevel.start_pos=sektor;
    for(i=0;i<POCET_POSTAV;i++)postavy[i].sektor=loadlevel.start_pos;
    SEND_LOG("(WIZARD) Load map '%s' %d",loadlevel.name,loadlevel.start_pos);
    unwire_proc();
    if (battle) konec_kola();
	battle=0;
	running_battle=0;
	doNotLoadMapState=1;
	hl_ptr=ikon_libs;
	destroy_fly_map();
	load_items();
	zneplatnit_block(H_ENEMY);
	zneplatnit_block(H_SHOP_PIC);
	zneplatnit_block(H_DIALOGY_DAT);
    load_shops();
    send_message(E_CLOSE_MAP);
  }
  return 0;
}

void enter_game(void)
  {
  int end;
  init_spectxtrs();
  chod_s_postavama(0);
  bott_draw(1);
  norefresh=0;
  task_wait_event(E_TIMER);
  redraw_scene();
  wire_main_functs();
  add_to_timer(TM_FAST_TIMER,2,-1,objekty_mimo);
  cancel_pass=0;
  set_game_click_map();
  if (autosave_on_enter) {
    autosave();
    autosave_on_enter = 0;
  }

  send_message(E_ADD,E_RELOADMAP,reload_map_handler);
  {
      EVENT_MSG *msg = task_wait_event(E_CLOSE_MAP);
      end = va_arg(msg->data, int);
  }
  send_message(E_DONE,E_RELOADMAP,reload_map_handler);

  unwire_main_functs();
  delete_from_timer(TM_FAST_TIMER);
  cancel_pass=1;
  task_wait_event(E_TIMER);
  task_wait_event(E_TIMER);
  unwire_main_functs();
  mute_all_tracks(1);
  if (end==255) konec_hry();
  }

/*int dos58(int mode);
#pragma aux dos58=\
  "mov  al,1"\
  "mov  ah,58h"\
  "int  21h"\
 parm[ebx] value [eax]
*/

/*
static int do_config_skeldal(int num,int numdata,char *txt)
  {
  switch (num)
     {
     case 0:vmode=numdata;break;
     case 1:zoom_speed(numdata);break;
     case 2:turn_speed(numdata);break;
     case 3:init_music_vol=numdata;break;
     case 4:init_gfx_vol=numdata;break;
     case 5:sscanf(txt,"%d %x %d %d",&snd_devnum,&snd_parm1,&snd_parm2,&snd_parm3);
            sound_detection=0;
            break;
     case 6:snd_mixing=numdata;break;
     case 7:strcopy_n(default_map,txt,20);default_map[19]='\0';break;
     case 8:gamespeed=numdata;break;
     case 9:level_preload=numdata;break;
//     case 10:system(txt);break;
     case 11:mman_patch=numdata;break;
     case 12:skip_intro=numdata;break;
     case 13:autosave_enabled=numdata;break;
     case 14: debug_enabled=numdata;break;
     case 15:full_video=numdata;break;
		 case 16:patch_file=getmem(strlen(txt)+1);
						 strcpy(patch_file,txt);
						 txt=strchr(patch_file,'\n');if (txt!=NULL) txt[0]=0;
						 break;
     case 17:titles_on=numdata;break;
     case 18:charmin=numdata;break;
     case 19:charmax=numdata;break;
	 case 20:game_extras=numdata;break;
	 case 21:windowed=numdata;break;
	 case 22:gamespeedbattle=numdata;break;
	 case 23:windowedzoom=numdata;break;
     case 24:monitor=numdata;break;
     case 25:if (VERSIONNUM<numdata)
               display_error("Pozor! Hra je starsi verze, nez vyzaduje dobrodruzstvi. Ve vlastnim zajmu si stahnete novou verzi, protoze toto dobrodruzstvi nemusi byt s aktualni verzi dohratelne");
            break;
     case 26:refresh=numdata;break;
     default:num-=CESTY_POS;
             gpathtable[num] = strdup(txt);
             SEND_LOG("(GAME) Directory '%s' has been assigned to group nb. %d",txt,num);
             break;

     }
 return 0;
  }
*/
/*
static void config_skeldal(const char *line)
  {
  int ndata=0,i,maxi;

  char *data=0;char *c;

  c=strchr(line,' ');if (c==NULL) return;
  c++;
  maxi=strlen(c);
  data=alloca(maxi+1);
  strcpy(data,c);
  while (maxi && (isspace(data[maxi-1]))) {
      --maxi;
      data[maxi]=0;
  }
  maxi=(sizeof(sinit)/sizeof(INIS));
  for(i=0;i<maxi;i++) if (comcmp(line,sinit[i].heslo)) break;
  if (i==maxi)
     {
     char s[256];
     i=data-line;

     strcpy(s,"Chyba v INI souboru: Neznama promenna - ");
     strncat(s,line,i);
     SEND_LOG("(ERROR) %s",s);
     }
  else
     {
     if (sinit[i].parmtype==INI_INT) if (sscanf(data,"%d",&ndata)!=1)
        {
        char s[256];

        sprintf(s,"Chyba v INI souboru: Ocekava se ciselna hodnota\n%s\n",line);
        SEND_LOG("(ERROR) %s",s);
        }
     do_config_skeldal(i,ndata,data);
     }
  }
*/
/*
static void configure(char *filename)
  {
  SEND_LOG("(GAME) Reading config. file '%s'",filename);
  cur_config=read_config(filename);
  if (cur_config==NULL)
     {
     char s[256];

     sprintf(s,"\nNemohu precist konfiguracni soubor \"%s\".\n",filename);
     SEND_LOG("(ERROR) %s",s);
     puts(s);
     exit(1);
     }

  process_ini(cur_config,config_skeldal);

  }
*/
static int update_config(void)
  {


  return 0;
  }

void help(void)
  {
  printf("Pouziti:\n\n   S <filename.MAP> <start_sector>\n\n"
         "<filename.MAP> jmeno mapy\n"
         "<start_sector> Cislo startovaciho sektoru\n"
         );
  exit(0);
  }

extern char nofloors;

/*
void set_verify(char state);
#pragma aux set_verify parm [eax]=\
                             "mov   ah,2eh"\
                             "int   21h"
*/
void play_movie_seq(const char *s,int y)
  {
  play_animation(s,0,y,0);
  }


void play_anim(int anim_num)
  {
     TSTR_LIST titl=NULL;
     const char *s = build_pathname(2,gpathtable[SR_VIDEO], texty[anim_num]);
     s = local_strdup(s);
     char *n = set_file_extension(s, ".TXT");
     if (load_string_list_ex(&titl,n)) titl=NULL;
     else {
         lang_patch_stringtable(&titl, "intro", "");
     }
     set_title_list(titl);set_font(H_FBIG,RGB(200,200,200));
     curcolor=0;bar32(0,0,639,459);
     showview(0,0,0,0);
     play_movie_seq(s,60);
     set_title_list(NULL);if (titl!=NULL) release_list(titl);
  }



#define V_NOVA_HRA 0
#define V_OBNOVA_HRY 1
#define V_UVOD 2
#define V_AUTORI 3
#define V_KONEC 4

#define H_ETOPBAR (H_MENUS_FREE+100)
#define H_EDESK (H_MENUS_FREE+101)
static void game_big_circle(char enforced)
  {
  int err;
  int r;
  char s[13];

  purge_playlist();
  strcopy_n(s,loadlevel.name,sizeof(s));
  err=load_map(s);
  if (!enforced)
     {
     loadlevel.start_pos=mglob.start_sector;
     loadlevel.dir=mglob.direction;
     }
  while (loadlevel.name[0])
     {
     if (err)
       {
	   char buff[256];

       switch (err)
          {
          case -1: sprintf(buff,"Error while loading map (%s) ....file not found\n",s);break;
          case -2: sprintf(buff,"Missing -1 at the end of map string table");break;
          case -3: sprintf(buff,"Map file is corrupted!\n");break;
          default: sprintf(buff,"Error in string table at line %d",err);break;
          }
	   display_error(buff);
       exit(1);
       }
    viewsector=loadlevel.start_pos;
    viewdir=loadlevel.dir;
    if (viewsector==0)
     {
     viewsector=set_leaving_place();
     if (viewsector==0)
        {
        viewsector=mglob.start_sector;
        viewdir=mglob.direction;
        }
     else
      {
      int i;
      viewdir=0;
      for (i=0;i<4;i++) if (~map_sides[i+(viewsector<<2)].flags & (SD_PLAY_IMPS | SD_PRIM_VIS))
         {viewdir=i;break;}
      }
     }
    for (int i = 0; i<POCET_POSTAV; ++i) {
        if (!postavy[i].used  || postavy[i].groupnum == cur_group) {
                postavy[i].inmaphash = current_map_hash;
        }
        if (postavy[i].sektor <0) postavy[i].sektor = -postavy[i].sektor;
        if (postavy[i].inmaphash != current_map_hash) {
            postavy[i].sektor = -postavy[i].sektor;
        }
    }
    recalc_volumes(viewsector,viewdir);
    for(r=0;r<mapsize*4;r++) call_macro(r,MC_STARTLEV);
    loadlevel.name[0]=0;
    reroll_all_shops();

    enter_game();

    leave_current_map();
    strcopy_n(s,loadlevel.name,sizeof(s));
    if (s[0]!=0) {
      err=load_map(s);
    }
    memset(GlobEventList,0,sizeof(GlobEventList));

    }

 }

extern THUMAN postavy_2[];

static void new_game(int argc, char *argv[])
  {
  int sect = 1,dir = 0;
  char enforce=0;

  purge_temps(0);
  game_time=0;
  reinit_kouzla_full();
  load_shops();
  open_story_file();
  if (argc<2)
     strcopy_n(loadlevel.name,default_map,sizeof(loadlevel.name));
  else
     strcopy_n(loadlevel.name,argv[1],sizeof(loadlevel.name));
  if (argc>2)
     {
     sscanf(argv[2],"%d",&sect);
     enforce=1;
     dir=0;
     }
  loadlevel.start_pos=sect;
  loadlevel.dir=dir;
  init_game();
  if (argc>=2)
     {
     memcpy(postavy,postavy_2,sizeof(THUMAN)*6);
     memset(runes,0x7f,sizeof(runes));
     }
  reg_grafiku_postav();
  memset(GlobEventList,0,sizeof(GlobEventList));
  game_big_circle(enforce);
  }

static void undef_menu(void)
  {
  int i;
  for(i=0;i<255;i++) undef_handle(0x8000+i);
  }


static void load_error_report(EVENT_MSG *msg,void **_)
  {
  if (msg->msg == E_IDLE)
     {
     message(1,0,0,"",texty[79],texty[80]);
     exit_wait=0;
     send_message(E_CLOSE_MAP);
     }
  }

static void wire_load_saved(void)
  {
  send_message(E_CLOSE_MAP,-1);
  }

static void load_saved_game(void)
  {
  char *game;

  err:
  loadlevel.name[0]=0;
  def_handle(H_ETOPBAR,"topbar_e.pcx",pcx_15bit_decomp,SR_BGRAFIKA);
  schovej_mysku();wire_proc=wire_load_saved;
  put_picture(0,0,ablock(H_ETOPBAR));
  put_picture(0,378,ablock(H_DESK));
  wire_save_load(4);
  ukaz_mysku();
  update_mysky();
  {
      EVENT_MSG *msg = task_wait_event(E_CLOSE_MAP);
      const char *cgame = msg?va_arg(msg->data, const char *):NULL;
      game = cgame?strdup(cgame):NULL;
  }
  unwire_proc();
  disable_click_map();
  task_wait_event(E_TIMER);
  if (game!=NULL)
      {
      reinit_kouzla_full();
      open_story_file();
	  memset(GlobEventList,0,sizeof(GlobEventList));
      if (load_game(game))
        {
        send_message(E_ADD,E_IDLE,load_error_report);
        task_wait_event(E_CLOSE_MAP);
        send_message(E_DONE,E_IDLE,load_error_report);
        exit_wait=0;
        free(game);
        goto err;
        }
      pick_set_cursor();
      undef_menu();
      init_game();
      build_all_players();
      game_big_circle(1);
      exit_wait=1;
      }
  free(game);
  }

static int any_save_callback(const char *c, LIST_FILE_TYPE _, size_t __, void *___) {
    return 1;
}

static char any_save() {
    int c = list_files(gpathtable[SR_SAVES],file_type_normal,any_save_callback,0);
    return c;
}


static void start(va_list args)
  {
  int volba;
  char /*d,*/openning;

   openning=0;
   update_mysky();
   schovej_mysku();
   if (!any_save())
    {
//    show_jrc_logo("LOGO.PCX");
    play_anim(7);
    }
   skip_intro=0;
   create_playlist(texty[1]);
   //play_next_music(&d);
   change_music(NULL);
   zobraz_mysku();
   showview(0,0,0,0);
  do
  {
     volba=enter_menu(openning);openning=1;
     switch (volba)
       {
       default:
       case V_KONEC:exit_wait=1;break;
       case V_NOVA_HRA: if (!enter_generator())
                          {
                          undef_menu();
                          new_game(0,NULL);
                          exit_wait=1;
                          }
                        break;
       case V_UVOD:bar32(0,0,639,479);
                   play_anim(7);
                   openning =0;
                   break;
       case V_OBNOVA_HRY:load_saved_game();break;
       case V_AUTORI:run_titles();break;
        }
     }
  while (!exit_wait);
  }

#if 0
static void start_from_mapedit(va_list args)
//#pragma aux start_from_mapedit parm[]
  {
  int argc=va_arg(args,int);
  char **argv=va_arg(args,char **);
  new_game(argc-1,argv+1);
  exit_wait=1;
  }
#endif

void disable_intro(void)
  {
  add_field_num(&cur_config,sinit[12].heslo,1);
  update_config();
  }

/*
 * -char def_path[]="";
-char graph_path[]="graphics" PATH_SEPARATOR;
-char basc_graph[]="graphics" PATH_SEPARATOR "basic" PATH_SEPARATOR;
-char item_graph[]="graphics" PATH_SEPARATOR "items" PATH_SEPARATOR;
-char sample_path[]="samples" PATH_SEPARATOR;
-char font_path[]="font" PATH_SEPARATOR;
-char map_path[]="maps" PATH_SEPARATOR;
-char music_path[]="music" PATH_SEPARATOR;
-char org_music_path[]="music" PATH_SEPARATOR;
-char temp_path[]="?";
-char enemies_path[]="graphics" PATH_SEPARATOR "enemies" PATH_SEPARATOR;
-char video_path[]="video" PATH_SEPARATOR;
-char dialogs_path[]="graphics" PATH_SEPARATOR "dialogs" PATH_SEPARATOR;
-char saves_path[]="";
-char work_path[]="";
-char cd_path[]="";
-char map2_path[]="";
-char plugins_path[]="";
 */

//returns game root
const char *configure_pathtable(const INI_CONFIG *cfg) {

#define DEFAULT_SUBPATHS 4
#define DEFAULT_SUBPATHS_LEN 20
    const char *sub_paths[DEFAULT_SUBPATHS] = {
            "basic", "dialogs", "enemies","items"
    };

    static char default_subpaths[DEFAULT_SUBPATHS][DEFAULT_SUBPATHS_LEN];
    for (int i = 0; i < DEFAULT_SUBPATHS; ++i) {
        strcopy_n(default_subpaths[i],  build_pathname(2, "graphics", sub_paths[i]), DEFAULT_SUBPATHS_LEN);
    }

    const INI_CONFIG_SECTION *paths = ini_section_open(cfg, "paths");
    const char *groot = ini_get_string(paths, "root", "./");
    gpathtable[SR_BGRAFIKA] = ini_get_string(paths, "gui", default_subpaths[0]);
    gpathtable[SR_DIALOGS] = ini_get_string(paths, "dialogs", default_subpaths[1]);
    gpathtable[SR_ENEMIES] = ini_get_string(paths, "enemies", default_subpaths[2]);
    gpathtable[SR_FONT] = ini_get_string(paths, "fonts", "font");
    gpathtable[SR_GRAFIKA] = ini_get_string(paths, "graphics", "graphics");
    gpathtable[SR_ITEMS] = ini_get_string(paths, "items", default_subpaths[3]);
    gpathtable[SR_MAP] = ini_get_string(paths, "maps", "maps");
    gpathtable[SR_MUSIC] = ini_get_string(paths, "music", "music");
    gpathtable[SR_ZVUKY] = ini_get_string(paths, "sounds", "sounds");
    gpathtable[SR_VIDEO] = ini_get_string(paths, "video", "video");
    gpathtable[SR_SAVES] = ini_get_string(paths, "savegame", get_default_savegame_directory());
    gpathtable[SR_DATA]= ini_get_string(paths, "data", "./");
    gpathtable[SR_LANG]= ini_get_string(paths, "lang", "./lang");
    const char *defmap = ini_get_string(paths, "default_map", NULL);
    if (defmap) {
        strcopy_n(default_map, defmap, sizeof(default_map));
    }
    patch_file = ini_get_string(paths, "patch_file", NULL);
    if (ini_get_boolean(paths, "patch_mode", 0)) {
        mman_patch = 1;
    }

    return groot;
}


static void (*display_error_cb)(const char *);
void display_error(const char *format, ...) {
    va_list lst;va_start(lst, format);
    if (display_error_cb) {
        char buff[1024];
        vsnprintf(buff,sizeof(buff), format, lst);
        display_error_cb(buff);
    } else {
        fprintf(stderr, format, lst);
    }
}

void quit_cb_exit_wait(void *_) {
    exit_wait = 1;
}


int skeldal_gen_string_table_entry_point(const SKELDAL_CONFIG *start_cfg, const char *save_path) {
    def_mman_group_table(gpathtable);

    INI_CONFIG *cfg = ini_open(start_cfg->config_path);
    if (cfg == NULL) {
        start_cfg->show_error(concat2("Failed to open configuration file: ", start_cfg->config_path));
        start_cfg->short_help();
        return 1;
    }

    if (start_cfg->adventure_path) {
        TSTR_LIST adv_config=read_config(start_cfg->adventure_path);
        adv_patch_config(cfg, adv_config);
        release_list(adv_config);
    }

    const char *groot = configure_pathtable(cfg);
    if (!change_current_directory(groot)) {
        start_cfg->show_error(concat2("Can't change directory to: ", groot));
        return 1;
    }

    init_DDL_manager();
    generate_string_tables(save_path);
    printf("Done\n");
    return 0;
}

void skeldal_entry_point_thread(va_list _) {
    int start_task = add_task(65536,start);

    escape();

    term_task_wait(start_task);
    return;
}

int skeldal_entry_point(const SKELDAL_CONFIG *start_cfg)
  {
  def_mman_group_table(gpathtable);

  display_error_cb = start_cfg->show_error;

  INI_CONFIG *cfg = ini_open(start_cfg->config_path);
  if (cfg == NULL) {
      start_cfg->show_error(concat2("Failed to open configuration file: ", start_cfg->config_path));
      start_cfg->short_help();
      return 1;
  }


  const char *groot = configure_pathtable(cfg);
  if (!change_current_directory(groot)) {
      start_cfg->show_error(concat2("Can't change directory to: ", groot));
      return 1;
  }

  if (start_cfg->adventure_path) {
      TSTR_LIST adv_config=read_config(start_cfg->adventure_path);
      if (!adv_config) {
          start_cfg->show_error(concat2("Failed to open adventure configuration: ", start_cfg->adventure_path));
          return 1;
      }
      adv_patch_config(cfg, adv_config);
      release_list(adv_config);
      configure_pathtable(cfg);
      char *sname = local_strdup(start_cfg->adventure_path);
      for (char *c = sname; *c; ++c) if (!isalnum(*c)) *c = '_';
      const char *p = build_pathname(2, gpathtable[SR_SAVES], sname);
      gpathtable[SR_SAVES] = local_strdup(p);
  } else if (start_cfg->lang_path) {
      lang_set_folder(build_pathname(2, gpathtable[SR_LANG], start_cfg->lang_path));
  }

  if (!start_cfg->adventure_path) {
    enable_achievements(1);
  }

  start_check();
  purge_temps(1);
  clrscr();

  int r =  init_skeldal(cfg, skeldal_entry_point_thread);
  ini_close(cfg);
  return r;


  }


#include "version.h"


int GetExeVersion(void)
  {
	return VERSIONNUM;
  }
