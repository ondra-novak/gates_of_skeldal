#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>


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
#include <libs/gui.h>
#include <libs/basicobj.h>

#include <string.h>
extern char enable_sort;
extern char autoattack;


char q_runsetup(char *parm)
  {
  char c[6];

  strcopy_n(c,parm,6);
  strupper(c);
  return !strncmp(c,"/SETUP",6);
  }


static void show_setup_desktop(WINDOW *w)
  {
  put_picture(w->x,w->y,ablock(H_SETUPBAR));
  }

static void checkbox_animator(THE_TIMER *t)
  {
  t;
  animate_checkbox(10,130,10);
  }

static int effects[]={SND_GVOLUME,SND_MUSIC,SND_GFX,SND_TREBL,SND_BASS};

static void do_setup_change(void)
  {
  char c;

  c=f_get_value(0,o_aktual->id);
  switch (o_aktual->id)
     {
     case 10:set_snd_effect(SND_SWAP,c & 1);break;
     case 20:set_snd_effect(SND_OUTFILTER,c & 1);break;
     case 250:timerspeed_val = TIMERSPEED- c;break;
     default:set_snd_effect(effects[o_aktual->id/10-20],c);break;
     }
  }

static void change_zoom(void)
  {
  int id=o_aktual->id;
  int i;

  for(i=30;i<60;i+=10) c_set_value(0,i,(f_get_value(0,i) & ~1) | (i==id));
  zoom_speed((id-30)/10);
  }

static void change_turn(void)
  {
  int id=o_aktual->id;
  int i;

  for(i=60;i<90;i+=10) c_set_value(0,i,(f_get_value(0,i) & ~1) | (i==id));
  turn_speed((id-60)/10);
  }

char exit_setup(int id,int xa,int ya,int xr,int yr);
static void unwire_setup(void);

static void setup_keyboard(EVENT_MSG *msg,void **_)
  {
  if (msg->msg == E_KEYBOARD)
     {
      int c= quit_request_as_escape(va_arg(msg->data, int));
      switch(c>>8) {
          case 1:
          case 59: exit_setup(0, 0, 0,0,0); break;
          case 60:clk_saveload(1, 0, 0, 0, 0);break;
          case 61:clk_saveload(0, 0, 0, 0, 0);break;
          case 64:go_book(0, 0, 0, 0, 0);break;
      }
     }
  }

static void wire_setup(void)
  {
  unwire_proc();
  unwire_proc=unwire_setup;
  mute_all_tracks(0);
  cur_mode=MD_SETUP;
  send_message(E_ADD,E_KEYBOARD,setup_keyboard);
  SEND_LOG("(GAME) Starting setup");
  }

static void unwire_setup(void)
  {
  show_names=f_get_value(0,90) & 1;
  enable_sort=f_get_value(0,100) & 1;
  autoattack=f_get_value(0,110) & 1;
  autosave_enabled=f_get_value(0,120) & 1;
  gamespeedbattle=(f_get_value(0,130) & 1)?GAMESPEED_FASTBATTLE:gamespeed;
  delete_from_timer(TM_CHECKBOX);
  mix_back_sound(32768);
  close_current();
  send_message(E_DONE,E_KEYBOARD,setup_keyboard);
  SEND_LOG("(GAME) Setup closed");
  }

char exit_setup(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya,xr,yr;
  unwire_setup();
  wire_proc();
  return 0;
  }

T_CLK_MAP setup[]=
  {
  {-1,0,0,639,14,exit_setup,2,H_MS_DEFAULT},
  {-1,0,15,639,479,exit_setup,0x8,H_MS_DEFAULT},
  };


void skeldal_checkbox(OBJREC *);
void setup_ok_button(OBJREC *);
void skeldal_soupak(OBJREC *);

static void exit_setup_action(void) {
  unwire_setup();
  wire_proc();
}



void new_setup()
  {
  WINDOW *w;
  CTL3D ctl;
  int i;
  static int textxp[]={ 75, 75,435,435,434,535,535,535,434,434,434,434,434, 35,410,510};
  static int textyp[]={275,305, 65, 95,125, 65, 95,125,185,215,245,275,305,235, 40, 40};
  static int  textc[]={ 53, 99, 56, 57, 58, 56, 57, 58,140,141,142,143,144 ,51, 55, 59};

  mix_back_sound(256000-16384);
  memset(&ctl,0,sizeof(ctl));
  change_click_map(setup,countof(setup));
  set_font(H_FBOLD,SETUP_COL2);
  default_font=curfont;
  memcpy(f_default,charcolors,sizeof(f_default));
  w=create_window(0,SCREEN_OFFLINE,639,359,0,&ctl);
  w->draw_event=show_setup_desktop;
  desktop_add_window(w);
  define(10,50,270,190,20,0,skeldal_checkbox);  c_default(get_snd_effect(SND_SWAP));   on_control_change(do_setup_change);
  if (check_snd_effect(SND_OUTFILTER))
     {
     define(20,50,300,190,20,0,skeldal_checkbox);c_default(get_snd_effect(SND_OUTFILTER));
        on_control_change(do_setup_change);
     }

  define(30,410,60,90,20,0,skeldal_checkbox);c_default(zoom_speed(-1)==0);
  on_control_change(change_zoom);
  define(40,410,90,90,20,0,skeldal_checkbox);c_default(zoom_speed(-1)==1);on_control_change(change_zoom);
  define(50,410,120,90,20,0,skeldal_checkbox);c_default(zoom_speed(-1)==2);on_control_change(change_zoom);

  define(60,510,60,90,20,0,skeldal_checkbox);c_default(turn_speed(-1)==0);on_control_change(change_turn);
  define(70,510,90,90,20,0,skeldal_checkbox);c_default(turn_speed(-1)==1);on_control_change(change_turn);
  define(80,510,120,90,20,0,skeldal_checkbox);c_default(turn_speed(-1)==2);on_control_change(change_turn);

  for(i=0;i<5;i++)
     {
     define((i+9)*10,410,180+i*30,190,20,0,skeldal_checkbox);
     switch(i)
        {
        case 0:c_default(show_names);break;
        case 1:c_default(enable_sort);break;
        case 2:c_default(autoattack);break;
        case 3:c_default(autosave_enabled);break;
        case 4:c_default(gamespeedbattle < gamespeed);break;
        }
     }

  for(i=0;i<(int)(sizeof(textc)/sizeof(int));i++)
     define(-1,textxp[i],textyp[i]-1,1,1,0,label,texty[textc[i]]);

  for(i=0;i<(int)(sizeof(effects)/sizeof(int));i++)
     if (check_snd_effect(effects[i]))
       {
       define(200+i*10,50+i*60,30,30,200,0,skeldal_soupak,effects[i]==SND_MUSIC?127:255);c_default(get_snd_effect(effects[i]));
       on_control_change(do_setup_change);
       }
    define(200+5*10,50+5*60,30,30,200,0,skeldal_soupak,TIMERSPEED-1);c_default(timerspeed_val >TIMERSPEED?0:TIMERSPEED - timerspeed_val);
    on_control_change(do_setup_change);

  define(300,559,336,81,21,0,setup_ok_button,texty[174]);on_control_change(exit_setup_action);
  property(NULL,ablock(H_FTINY),&color_topbar,0);
  redraw_window();
  add_to_timer(TM_CHECKBOX,4,-1,checkbox_animator);
  }

void game_setup_(void)
  {
  wire_setup();
  new_setup();
  }


char game_setup(int id,int xa,int ya,int xr,int yr)
  {
  id;xa;ya;xr;yr;
  run_background(game_setup_);
  return 1;
  }


void GameResume(EVENT_MSG *msg,void **data)
  {
  static int volsave;
  data;
  if (msg->msg==E_INIT)
     {
     volsave=get_snd_effect(SND_GVOLUME);
     set_snd_effect(SND_GVOLUME,volsave>>1);
     }
  if (msg->msg==E_KEYBOARD)
     {
     set_snd_effect(SND_GVOLUME,volsave);
     wire_main_functs();
     msg->msg=-2;
     }
  }

void GamePause()
  {
  int i;
  unwire_proc();
  send_message(E_ADD,E_KEYBOARD,GameResume);
  update_mysky();
  schovej_mysku();
  trans_bar(0,0,640,480,0);
  set_font(H_FBOLD,RGB555(0,23,0));
  i=text_width(texty[5]);
  add_window(320-(i/2)-10,100,i+40,40,H_IDESKA,4,20,20);
    redraw_window();
  set_aligned_position(320,115,1,1,texty[5]);
  outtext(texty[5]);
  ukaz_mysku();
  showview(0,0,0,0);
  }

/*void user_setup()
  {

  initmode256(cur_xlat);
  init_mysky();
  hranice_mysky(0,0,639,479);
  mouse_set_default(H_MS_DEFAULT);
  ukaz_mysku();
  setup_dialoge();
  escape();
  schovej_mysku();
  closemode();
  }
 */
