#include <platform.h>
#include <stdio.h>
#include <types.h>
#include <event.h>
#include <devices.h>
#include <bgraph.h>
#include <bmouse.h>
#include <memman.h>
#include "globals.h"
#include "engine1.h"

#define get_shift_state() ((GetKeyState(VK_SHIFT) & 0x80)!=0)
int default_ms_cursor=0;

char spell_cast=0;

char clk_step(int id,int xa,int ya,int xr,int yr)
  {
  xa;ya;xr;yr;
  spell_cast=0;
  pick_set_cursor();
  switch (id)
     {
        case H_SIPKY_S:step_zoom(0);break;
        case H_SIPKY_J:step_zoom(2);break;
        case H_SIPKY_SV:turn_zoom(1);break;
        case H_SIPKY_SZ:turn_zoom(-1);break;
        case H_SIPKY_Z:step_zoom(3);break;
        case H_SIPKY_V:step_zoom(1);break;
        default:return 0;
     }
  return 1;
  }

char clk_touch_vyk(int sector,int side,int xr,int yr)
  {
  int i;
  TVYKLENEK *v;
  int x1,y1,x2,y2;

  if (picked_item!=NULL && picked_item[1]!=0) return 0;
  for(i=0;v=map_vyk+i,i<vyk_max;i++) if (v->sector==sector && v->dir==side) break;
  if (i==vyk_max) return 0;
  x1=v->xpos-v->xs/2;
  x2=v->xpos+v->xs/2;
  y1=320-(v->ypos+v->ys);
  y2=320-(v->ypos);
  x1+=MIDDLE_X-points[0][1][1].x;
  x2+=MIDDLE_X-points[0][1][1].x;
  y1+=MIDDLE_Y+points[0][1][1].y;
  y2+=MIDDLE_Y+points[0][1][1].y;
  if (x1<=xr && xr<=x2 && y1<=yr && yr<=y2)
     {
     if (picked_item==NULL)
        {
        for(i=0;v->items[i];i++);
        if (!i) return 0;
        i--;
        picked_item=NewArr(short,2);
        picked_item[0]=v->items[i];picked_item[1]=0;
        v->items[i]=0;
        do_items_specs();
        pick_set_cursor();
        call_macro(viewsector*4+viewdir,MC_VYKEVENT);
        return 1;
        }
     else
        {
        int fc;
        word *w;
        for(i=0;v->items[i];i++);
        if (i==8)
           {
           bott_disp_text(texty[36]);
           return 1;
           }
        fc=picked_item[0];fc=glob_items[fc-1].vzhled+face_arr[0];
        w=ablock(fc);
        if (v->xs<w[0] || v->ys<w[0])
           {
           bott_disp_text(texty[35]);
           return 1;
           }
        v->items[i]=picked_item[0];
        v->items[i+1]=0;
        call_macro(viewsector*4+viewdir,MC_VYKEVENT);
        free(picked_item);picked_item=NULL;
        pick_set_cursor();
        return 1;
        }
     }
  return 0;
  }

char clk_touch(int id,int xa,int ya,int xr,int yr)
  {
  int x1=0,y1=0,x2=0,y2=0;
  word *p;
  int ext=0;

  xa;ya;id;
  spell_cast=0;
  pick_set_cursor();
  id=viewsector*4+viewdir;
  if (map_sides[id].oblouk & 0x10) if (clk_touch_vyk(viewsector,viewdir,xr,yr))return 1;
  if (map_sides[id].flags & SD_SEC_VIS && map_sides[id].sec!=0)
     {
     xa=map_sides[id].xsec<<1;
     ya=320-(map_sides[id].ysec<<1);
     p=(word *)ablock(map_sides[id].sec+num_ofsets[MAIN_NUM]);
     x1=*p++;y1=*p++;
     x2=xa+x1/2;
     y2=ya+y1/2;
     y1=y2-y1;
     x1=x2-x1;
     x1+=MIDDLE_X-points[0][1][1].x;
     x2+=MIDDLE_X-points[0][1][1].x;
     y1+=MIDDLE_Y+points[0][1][1].y;
     y2+=MIDDLE_Y+points[0][1][1].y;
     ext=1;
     }
  else if (map_sides[id].sec==0)
     {
     x1=MIDDLE_X-points[0][0][1].x;
     y1=MIDDLE_Y+points[0][1][1].y;
     x2=640-x1;
     y2=MIDDLE_Y+points[0][0][1].y;
     ext=((map_sides[id].flags & SD_THING_IMPS) && !(map_sides[id].oblouk & SD_ITPUSH));
     }
  else return 0;
  if (x1<=xr && xr<=x2 && y1<=yr && yr<=y2)
     {
     a_touch(viewsector,viewdir);
     return ext;
     }
  return 0;
 }

char clk_fly_cursor(int id,int xa,int ya,int xr,int yr)
  {
  id;

  if (spell_cast)
     {
     spell_cast=0;
     pick_set_cursor();
     return 1;
     }
  if (yr>180)
     if(xr>480) clk_step(H_SIPKY_V,xa,ya,xr,yr);
     else if(xr<160) clk_step(H_SIPKY_Z,xa,ya,xr,yr);
     else clk_step(H_SIPKY_J,xa,ya,xr,yr);
  else
     if(xr>480) clk_step(H_SIPKY_SV,xa,ya,xr,yr);
     else if(xr<160) clk_step(H_SIPKY_SZ,xa,ya,xr,yr);
     else clk_step(H_SIPKY_S,xa,ya,xr,yr);
  return 1;
  }

char clk_throw(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya;
  if (picked_item!=NULL)
     {
     throw_fly(xr,yr,0);
     return 1;
     }
  return 0;
  }

char go_map(int id,int xa,int ya,int xr,int yr)
  {
  int i=15*256;
  id;xa;ya;xr;yr;

  if (cur_mode==MD_ANOTHER_MAP)
     {unwire_proc();wire_proc();return 1;}
  spell_cast=0;
  pick_set_cursor();
  send_message(E_KEYBOARD,i);
  return 1;
  }

char go_book(int id,int xa,int ya,int xr,int yr)
  {
  id;xa;ya;xr;yr;

  if (cur_mode==MD_ANOTHER_MAP) unwire_proc(),wire_proc();
  spell_cast=0;
  pick_set_cursor();
  wire_kniha();
  return 1;
  }


char konec(int id,int xa,int ya,int xr,int yr)
 {
 id;xa;ya;xr;yr;
 unwire_proc();
 if (!message(2,0,1,texty[118],texty[76],texty[77],texty[78]))
  {
  if (cur_mode==MD_ANOTHER_MAP) unwire_proc(),wire_proc();
  send_message(E_CLOSE_MAP);
  }
 else wire_proc();
 return 1;
 }

char spell_casting(int id,int xa,int ya,int xr,int yr)
 {
 id;xa;ya;xr;yr;
 if (picked_item!=NULL) return 1;
 spell_cast=1;
 mouse_set_default(H_MS_WHO);
 return 1;
 }


char clk_sleep(int id,int xa,int ya,int xr,int yr)
  {
  id;xa;ya;xr;yr;
  if (cur_mode==MD_ANOTHER_MAP) unwire_proc(),wire_proc();
  if (mglob.map_effector==ME_MESTO)
     {
     bott_disp_text(texty[120]);
     return 1;
     }
  if (!battle) add_task(8100,sleep_players);
  return 1;
  }


char start_invetory(int id,int xa,int ya,int xr,int yr)
  {
  THUMAN *p;
  int i;
  word *xs;

  id;xa;ya;yr;
  if (cur_mode==MD_ANOTHER_MAP) unwire_proc(),wire_proc();
  if (bott_display!=BOTT_NORMAL)
     {
     schovej_mysku();
     bott_draw(1);
     other_draw();
     ukaz_mysku();
     showview(0,376,640,104);
     return 1;
     }
  xs=ablock(H_OKNO);
  i=xr/xs[0];
  if (i<POCET_POSTAV)
     {
     i=group_sort[i];
     p=&postavy[i];
     if (p->used)
        {
        if (ms_last_event.event_type & 0x2)
           {
           if (get_control_key_state())
              {
              if (p->sektor==viewsector)
                 {
                 add_to_group(i);
                 zmen_skupinu(p);
                 return 1;
                 }
              return 1;
              }
           if (select_player!=i && select_player!=-1 && picked_item==NULL && !(cur_mode==MD_INV && battle_mode==MD_PREZBROJIT))
                 {
                 select_player=i;
                 zmen_skupinu(p);
                 bott_draw(1);
                 if (cur_mode==MD_INBATTLE) return 1;
                 }
           if (p->groupnum!=cur_group || spell_cast)
              {
              if (picked_item!=NULL && p->sektor!=viewsector) return 0;
              if (spell_cast && p->sektor==viewsector)
                 {
                 wire_fly_casting(i);
                 spell_cast=0;
                 mouse_set_default(H_MS_DEFAULT);
                 return 1;
                 }
              zmen_skupinu(p);
              if (cur_mode==MD_GAME && p->groupnum) return 1;
              }
           if (!p->groupnum && p->sektor!=viewsector) return 1;
           unwire_proc();
           wire_inv_mode(p);
           return 0;
           }
        else
           {
           if (picked_item==NULL) return 0;
           if (p->sektor==viewsector)
           if (put_item_to_inv(p,picked_item))
              {
              free(picked_item);
              picked_item=NULL;
              }
           pick_set_cursor();
           if (cur_mode==MD_INV)
             {
             unwire_proc();
             wire_inv_mode(human_selected);
             }
           return 1;
           }
         }
     }
  return 0;
  }

char clk_group_all(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya,xr,yr;
  unwire_proc();
  wire_proc();
  group_all();
  return 1;
  }

char clk_saveload(int id,int xa,int ya,int xr,int yr)
  {
  xa;ya;xr;yr;
  if (id && battle)
     {
     bott_disp_text(texty[81]);other_draw();showview(0,0,0,0);
     return 1;
     }
  if (id && !GlobEvent(MAGLOB_CLICKSAVE,viewsector,viewdir))
	  return 1;
  if (cur_mode==MD_ANOTHER_MAP) unwire_proc(),wire_proc();
  unwire_proc();
  cancel_render=1;
  wire_save_load(id);
  return 1;
  }

char return_game(int id,int xa,int ya,int xr,int yr)
  {
  id;xa;ya;xr;yr;
  if (ms_last_event.event_type & 0x8)
     {
     unwire_proc();
     wire_proc();
     }
  return 0;
  }

char clk_mob_alter(int id,int xa,int ya,int xr,int yr)
  {
  id;xa;ya;xr;yr;

  xa=viewsector;
  id=mob_map[xa];
  while (id==0 || mobs[id-1].dialog==-1)
     {
     if (id) id=mobs[id-1].next;
     if (id==0 && xa==viewsector)
        if (~map_sides[(xa<<2)+viewdir].flags & SD_PLAY_IMPS)
           {
           xa=map_sectors[xa].step_next[viewdir];
           id=mob_map[xa];
           }
        else return 0;
     else return 0;
     }
  id--;
  start_dialog(mobs[id].dialog,id);
  return 1;
  }

char empty_clk(int id,int xa,int ya,int xr,int yr) //tato udalost slouzi ke zruseni nekterych mist v globalni mape
  {
  id,xa,ya,xr,yr;
  return 1;
  }

static char sing_song_clk(int id,int xa,int ya,int xr,int yr)
  {
  char *xadr;
  word *xs;
  static char playing=0;
  char standardflute=map_sectors[viewsector].sector_type>=S_FLT_SMER &&
            map_sectors[viewsector].sector_type<S_FLT_SMER+4;

  id,xa,ya,xr,yr;


  if (bott_display!=BOTT_FLETNA) return 0;
  xadr=ablock(H_FLETNA_MASK);
  xs=(word *)xadr;
  xadr+=6;
  xadr+=*xs*yr+xr;
  id=*xadr;
  id--;
  if (id<0 || id>12 || (ms_last_event.event_type & 0x4))
     {
     if (playing)
        {
        THE_TIMER *t;
        stop_play_flute();
        playing=0;
        if (standardflute)
        {
        if (fletna_get_buffer_pos())
           {
           THE_TIMER *t;
           if (standardflute)
           {
            t=add_to_timer(TM_FLETNA,100,1,check_fletna);
            t->userdata[0]=viewsector;
            t->userdata[1]=viewdir;
           }
           }
        }
           else
           {
            t=add_to_timer(TM_FLETNA,100,1,check_global_fletna);
            t->userdata[0]=viewsector;
            t->userdata[1]=viewdir;
           }

        }
     return id<12;
     }
  if (~ms_last_event.event_type & 0x2) return 1;
  start_play_flute(id);
  playing=1;
  if (standardflute)
            {
              fletna_pridej_notu(id);
              delete_from_timer(TM_FLETNA);
            }
  else
  {
    fletna_glob_add_note(id);
    delete_from_timer(TM_FLETNA);
  }
  return 1;
  }


T_CLK_MAP clk_main_view[]=
  {
  {H_SIPKY_S,561,378,598,407,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_SZ,530,387,560,418,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_Z,529,419,555,453,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_J,560,446,598,474,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_SV,599,387,628,418,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_V,605,420,632,454,clk_step,2,H_MS_DEFAULT},
  {MS_GAME_WIN,0,17,639,377,clk_fly_cursor,8,-1},
  {1,320,303,639,376,pick_item_,2,-1},//344
  {0,0,303,320,376,pick_item_,2,-1},//344
  {3,0,200,320,340,pick_item_,2,-1},//303
  {2,320,200,639,340,pick_item_,2,-1},//303
  {MS_GAME_WIN,0,17,639,377,clk_touch,2,-1},
  {MS_GAME_WIN,0,17,639,377,clk_throw,2,-1},
  {MS_GAME_WIN,0,17,639,250,clk_mob_alter,2,-1},
  {-1,0,378,639,479,sing_song_clk,0xff,-1},
  {-1,54,378,497,479,start_invetory,2+8,-1},
  {-1,315,0,335,14,spell_casting,2,-1},
  };

#define GAME_CLK_MAP 8
T_CLK_MAP game_clk_map[]=
  {
  {-1,499,379,518,390,clk_group_all,2,-1},
  {-1,337,0,357,14,go_map,2,H_MS_DEFAULT},
  {-1,291,0,313,14,go_book,2,H_MS_DEFAULT},
  {-1,87,0,142,14,game_setup,2,H_MS_DEFAULT},
  {-1,30,0,85,14,konec,2,H_MS_DEFAULT},
  {1,147,0,205,14,clk_saveload,2,H_MS_DEFAULT},
  {0,207,0,265,14,clk_saveload,2,H_MS_DEFAULT},
  {-1,267,0,289,15,clk_sleep,2,H_MS_DEFAULT},
  };


T_CLK_MAP *click_map=NULL;
int click_map_size=0;
T_CLK_MAP *global_click_map=NULL;
int global_click_map_size=0;

int find_in_click_map(MS_EVENT *ms,T_CLK_MAP *pt,int pocet,int *evtype)
  {
  int mscur=-1;
  while (pocet--)
     {
     if (ms->x>=pt->xlu && ms->x<=pt->xrb && ms->y>=pt->ylu && ms->y<=pt->yrb)
        {
        if (mscur==-1) mscur=pt->cursor;
        if ((*evtype) & pt->mask)
         if (pt->proc!=NULL)
           if (pt->proc(pt->id,ms->x,ms->y,ms->x-pt->xlu,ms->y-pt->ylu))
              {
              evtype[0]&=~pt->mask;
              if (!evtype[0]) break;
              }
        }
     pt++;
     }
  return mscur;
  }


void ms_clicker(EVENT_MSG *msg,void **usr)
  {
  MS_EVENT *ms;

  usr;
  if (pass_zavora) return;
  switch (msg->msg)
     {
     case E_INIT:return;
     case E_DONE:return;
     case E_MOUSE:
           {
           int mscur;
           int msc=-1;
           int evtype;
           ms=get_mouse(msg);
           mscur=-1;
           evtype=ms->event_type;
           mscur=find_in_click_map(ms,click_map,click_map_size,&evtype);
           if (evtype) msc=find_in_click_map(ms,global_click_map,global_click_map_size,&evtype);
           if (mscur==-1) mscur=msc;
           if (mscur!=-1) mouse_set_cursor(mscur);
           else mouse_set_cursor(default_ms_cursor);
           }
          break;
     }
  return;
  }

void change_click_map(T_CLK_MAP *map,int mapsize)
  {
  click_map=map;
  click_map_size=mapsize;
  }

void save_click_map(void **map,int *mapsize)
  {
  *map=click_map;
  *mapsize=click_map_size;
  }

void restore_click_map(void *map,int mapsize)
  {
  click_map=(T_CLK_MAP *)map;
  click_map_size=mapsize;
  }



void change_global_click_map(T_CLK_MAP *map,int mapsize)
  {
  global_click_map=map;
  global_click_map_size=mapsize;
  }

void set_game_click_map(void)
  {
  change_global_click_map(game_clk_map,GAME_CLK_MAP);
  }


T_CLK_MAP map_disabled=
  {-1,0,0,639,479,empty_clk,0xff,-1};

void disable_click_map(void)
  {
  change_click_map(&map_disabled,1);
  }
