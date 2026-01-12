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
#include <libs/strlite.h>
#include "engine1.h"
#include <libs/pcx.h>
#include "globals.h"

#include <ctype.h>
#include <string.h>
#define AUTOMAP_BACK RGB555(8,4,0)
#define AUTOMAP_VODA RGB555(0,15,31)
#define AUTOMAP_LAVA RGB555(31,16,0)
#define AUTOMAP_FORE RGB555(18,17,14)
#define AUTOMAP_FORE_LVP RGB555(14,13,11)
#define AUTOMAP_BKG RGB888(164,160,147)
#define AUTOMAP_LINE1 RGB555(13,11,10)
#define AUTOMAP_LINE2 RGB555(31,22,6)
#define AUTOMAP_MOB RGB555(31,8,8)

#define MEDIUM_MAP 9
#define MEDIUM_MMAP 4
#define MEDIUM_MAP_COLOR RGB555(21,20,19)
#define MEDIUM_MAP_LINE1 RGB555(25,20,17)
#define MEDIUM_MAP_LINE2 RGB555(31,22,6)

#define AUTOMAP_FONT_COLOR (RGB555(0,0,0)|FONT_TSHADOW_GRAY)

word stairs_colors[7]=
  {AUTOMAP_LINE1,
   RGB555(14,12,11),
   RGB555(15,14,12),
   RGB555(16,15,12),
   RGB555(17,16,13)};

word arrow_colors[7]=
  {
  AUTOMAP_LINE1,
  AUTOMAP_FORE
  };

char shift_map(int id,int xa,int ya,int xr,int yr);
char psani_poznamek(int id,int xa,int ya,int xr,int yr);
char map_target_select(int id,int xa,int ya,int xr,int yr);
char map_target_cancel(int id,int xa,int ya,int xr,int yr);
char map_menu(int id,int xa,int ya,int xr,int yr);
char map_menu_glob_map(int id,int xa,int ya,int xr,int yr);


char enable_glmap=0;

static int map_xr,map_yr;
static int cur_depth;
static TSTR_LIST texty_v_mape=NULL;

#define BOTT 378
#define LEFT 520

static char cur_disables;

#define CLK_MAP_VIEW 5
T_CLK_MAP clk_map_view[]=
  {
  {MS_GAME_WIN,0,17,639,377,psani_poznamek,2,H_MS_DEFAULT},
  {-1,54,378,497,474,start_invetory,2+8,-1},
  {-1,337,0,357,14,go_map,2,H_MS_DEFAULT},
  {-1,LEFT,BOTT,639,479,map_menu,2,H_MS_DEFAULT},
  {-1,0,0,639,479,return_game,8,-1},
  };

#define CLK_GLOB_MAP 4
T_CLK_MAP clk_glob_map[]=
  {
  {-1,54,378,497,474,start_invetory,2+8,-1},
  {-1,337,0,357,14,go_map,2,H_MS_DEFAULT},
  {-1,LEFT,BOTT,639,479,map_menu,2,H_MS_DEFAULT},
  {-1,0,0,639,479,map_menu_glob_map,8,-1},
  };

#define CLK_TELEPORT_VIEW 4
T_CLK_MAP clk_teleport_view[]=
  {
  {MS_GAME_WIN,0,17,639,377,map_target_select,2,H_MS_DEFAULT},
  {-1,LEFT,BOTT,639,479,map_target_cancel,2,H_MS_DEFAULT},
  {-1,0,0,639,479,map_target_cancel,8,-1},
  {-1,0,0,639,479,empty_clk,0xff,-1},
  };

char testclip(int x,int y)
  {
  return (y>=16 && y<360+16 && x>=8 && x<630);
  }

/*void shift_map_event(EVENT_MSG *msg,int *data)
  {
  static int smer;

  data;
  if (msg->msg==E_INIT) smer=*(int *)msg->data;
  else if (msg->msg==E_TIMER)
        {
        switch (smer)
           {
           case H_SIPKY_S:send_message(E_KEYBOARD,'H'*256);break;
           case H_SIPKY_J:send_message(E_KEYBOARD,'P'*256);break;
           case H_SIPKY_V:send_message(E_KEYBOARD,'M'*256);break;
           case H_SIPKY_Z:send_message(E_KEYBOARD,'K'*256);break;
           case H_SIPKY_SZ:send_message(E_KEYBOARD,'s'*256);msg->msg=-2;break;
           case H_SIPKY_SV:send_message(E_KEYBOARD,'t'*256);msg->msg=-2;break;
           }
        }
   else if (msg->msg==E_MOUSE)
     {
     MS_EVENT *ms;

     ms=get_mouse(msg);
     if (!ms->tl1 && !ms->tl2)
        {
        send_message(E_DONE,E_TIMER,shift_map_event);
        msg->msg=-2;
        schovej_mysku();
        other_draw();
        ukaz_mysku();
        showview(0,0,0,0);
        }
     }
  }
 */

void save_text_to_map(int x,int y,int depth,char *text)
  {
  char c[512],*d;
  if (text[0]==0) return;
  d = text;
  while (*d && isspace(*d)) ++d;
  if (!*d)  return;
  memset(c,1,sizeof(c));
  strncpy(c+12,text,sizeof(c)-13);
  c[511] = 0;
  if (texty_v_mape==NULL) texty_v_mape=create_list(8);
  int id = str_add(&texty_v_mape,c);
  d=texty_v_mape[id];
  x=(x-320)+map_xr;
  y=(y-197)+map_yr;
  memcpy(d,&x,4);memcpy(d+4,&y,4);memcpy(d+8,&depth,4);
  }

static char check_for_layer(int layer)
  {
  int i;
  TMAP_EDIT_INFO *m=map_coord;

  for(i=0;i<mapsize;i++,m++) if (m->layer==layer && m->flags & MC_MARKED && m->flags & MC_AUTOMAP) return 1;
  return 0;
  }

void ukaz_vsechny_texty_v_mape(void)
  {
  int x,y,d,i,cn;
  char *c;

  if (texty_v_mape==NULL) return;
  set_font(H_FLITT5,AUTOMAP_FONT_COLOR);
  cn=str_count(texty_v_mape);
  for(i=0;i<cn;i++)
     {
     c=texty_v_mape[i];
     if (c!=NULL)
        {
        memcpy(&x,c,4);memcpy(&y,c+4,4);memcpy(&d,c+8,4);c+=12;
        x=x-map_xr;
        y=y-map_yr;
        x+=320;y+=197;
        if (d==cur_depth) {
        if (testclip(x,y))
           {
           int h;char *d,e;
           d=strchr(c,0);e=0;
           while((h=text_width(c)+x)>640)
              {
              *d=e;
              d--;e=*d;*d=0; if (d==c) break;
              }
           position(x,y);outtext(c);
           *d=e;
           }
        else if(x<8 && (x+text_width(c)>10) && y>16 && y<376)
           {
           char cd[2]=" ";
           while (x<10 && *c)
              {
              cd[0]=*c++;x+=text_width(cd);
              }
           position(x,y);outtext(c);
           }
        }
        }
     }
  }

void psani_poznamek_event(EVENT_MSG *msg,void **data)
  {
  static int x,y, index;
  static char text[255];
  static char *save;
  static void *save_pic=NULL;

  data;
  if (msg->msg==E_INIT)
     {
      char *c;
     x = va_arg(msg->data, int);
     y = va_arg(msg->data, int);
     c = va_arg(msg->data, char *);

     set_font(H_FLITT5,AUTOMAP_FONT_COLOR);
     strcpy(text,c);
     save=(char *)getmem(strlen(text)+1);
     strcpy(save,text);
     index=strchr(text,0)-text;
     if (save_pic==NULL)
        {
        schovej_mysku();
        save_pic=getmem(640*20*2+6);
        get_picture(0,y,640,20,save_pic);
        position(x,y);outtext(text);outtext("_");
        showview(0,0,640,480);
        }
     }
  else
  if (msg->msg==E_MOUSE)
     {
     MS_EVENT *ms;

     ms=get_mouse(msg);
     if (ms->event_type & 0x8) send_message(E_KEYBOARD,27);
     msg->msg=-1;
     }
  else if (msg->msg==E_KEYBOARD)
     {
     char c;

     c=quit_request_as_escape(va_arg(msg->data, int));
     set_font(H_FLITT5,AUTOMAP_FONT_COLOR);
     if (c)
        {
        switch (c)
           {
           case 8:if (index) index--; text[index]=0;break;
           case 27:strcpy(text,save);
                   CASE_FALLTHROUGH;
           case 13:save_text_to_map(x,y,cur_depth,text);
                  send_message(E_DONE,E_MOUSE,psani_poznamek_event);
                  msg->msg=-2;
                  return;
           default:if (c>=32)
              {
              text[index]=c;
              text[index+1]=0;
              if (text_width(text)>(640-x)) text[index]=0; else index++;
              }
           }
        put_picture(0,y,save_pic);
        position(x,y);outtext(text);outtext("_");
        showview(x,y,640,20);
        }
     msg->msg=-1;
     }
  else if (msg->msg==E_DONE)
     {
     if (save_pic!=NULL)
        {
        put_picture(0,y,save_pic);
        showview(x,y,640,y+20);
        free(save_pic);save_pic=NULL;
        send_message(E_AUTOMAP_REDRAW);
        ukaz_mysku();
        }
     if (save!=NULL)
        {
        free(save);save=NULL;
        }
     }
  }

int hledej_poznamku(int x,int y,int depth)
  {
  int i,count;

  x=(x-320)+map_xr;
  y=(y-197)+map_yr;
  if (texty_v_mape==NULL) return -1;
  count=str_count(texty_v_mape);
  set_font(H_FLITT5,AUTOMAP_FONT_COLOR);
  for(i=0;i<count;i++)
     if (texty_v_mape[i]!=NULL)
        {
        int xa,ya,xb,yb,dep;
        int xas,yas,xbs,ybs,xs,ys;

        xa=*(int *)(texty_v_mape[i]);
        ya=*(int *)(texty_v_mape[i]+4);
        dep=*(int *)(texty_v_mape[i]+8);
        xs=text_width(texty_v_mape[i]+12);
        ys=text_height(texty_v_mape[i]+12);
        xb=xa+xs;
        yb=ya+ys;
        if (x>=xa && y>=ya && x<=xb && y<=yb && dep==depth)
           {
           xas=(xa+320)-map_xr;
           yas=(ya+197)-map_yr;
           xbs=xas+xs;
           ybs=yas+ys;
           if (xas>0 && xbs<640 && yas>16 && ybs<360+16) return i;
           return -2;
           }
        }
  return -1;
  }

char psani_poznamek(int id,int xa,int ya,int xr,int yr)
  {
  xa;ya;xr;yr;

  if (cur_mode == MD_ANOTHER_MAP) return 0;
  if ((id=hledej_poznamku(xa,ya,cur_depth))==-1)
     {
     xa&=~7;xa+=2;
     ya&=~7;ya-=4;
     send_message(E_ADD,E_KEYBOARD,psani_poznamek_event,xa,ya,"");
     send_message(E_ADD,E_MOUSE,psani_poznamek_event,xa,ya,"");
     }
  else if (id!=-2)
     {
     char *s;

     xa=*(int *)(texty_v_mape[id]);
     ya=*(int *)(texty_v_mape[id]+4);
     xa=(xa+320)-map_xr;
     ya=(ya+197)-map_yr;
     s=(char *)getmem(strlen(texty_v_mape[id]+12)+1);
     strcpy(s,texty_v_mape[id]+12);
     str_remove(&texty_v_mape,id);
     str_delfreelines(&texty_v_mape);
     send_message(E_AUTOMAP_REDRAW);
     for(xr=0;xr<10;xr++) do_events();
     send_message(E_ADD,E_KEYBOARD,psani_poznamek_event,xa,ya,s);
     send_message(E_ADD,E_MOUSE,psani_poznamek_event,xa,ya,s);
     free(s);
     }
  return 0;
  }

char shift_map(int id,int xa,int ya,int xr,int yr)
  {
  id;xa;ya;xr;yr;

  anim_sipky(id,1);
//  send_message(E_ADD,E_TIMER,shift_map_event,id);
//  send_message(E_ADD,E_MOUSE,shift_map_event,id);
  return 0;
  }

static void print_symbol(int x,int y,char znak)
  {
  char c[2]=" ";
  position(x+1,y+1);c[0]=znak;outtext(c);
  }


static void draw_amap_sector(int x,int y,int sector,int mode,int turn,int line1,int line2)
  {
  int j,i,k;
  TSTENA *q;
  TSECTOR *ss;

        q=&map_sides[sector<<2];
        ss=&map_sectors[sector];
        if (ss->sector_type==S_VODA || ss->sector_type==S_LODKA) curcolor=AUTOMAP_VODA;
        else if (ss->sector_type==S_LAVA) curcolor=AUTOMAP_LAVA;
        else if (ss->sector_type==S_LEAVE) curcolor=AUTOMAP_FORE_LVP;
        else curcolor=AUTOMAP_FORE;
        if (!mode)
           {
           trans_bar(x,y,8,8,curcolor);
           if ((k=map_coord[sector].flags & 0x600)!=0)
              {
              int i;

              i=map_sectors[sector].sector_type;
              set_font(H_FSYMB,0);k>>=9;
              switch (k)
                 {
                 default:
                 case 3:break;
                 case 2:set_font(H_FSYMB,0);
                        font_color(stairs_colors);
                        print_symbol(x,y,'s');break;
                 case 1:set_font(H_FSYMB,0);
                        font_color(arrow_colors);
                        print_symbol(x,y,4+((i-S_SMER+4-turn) & 0x3));break;

                 }
              }
           else
           switch(map_sectors[sector].sector_type)
              {
              int i;
              TSTENA *sd;
              case S_SCHODY:set_font(H_FSYMB,0x3e0);
                            memcpy(charcolors,stairs_colors,sizeof(stairs_colors));
                            print_symbol(x,y,'s');break;
              case S_TELEPORT:
                  for(i=0,sd=map_sides+sector*4;i<4 && ~sd->flags & SD_SEC_VIS;i++,sd++) {}
                  if (i!=4) {set_font(H_FSYMB,0x3e0);print_symbol(x,y,'T');}
                  break;
              case S_DIRA:set_font(H_FSYMB,AUTOMAP_FONT_COLOR);print_symbol(x,y,'N');break;
              }
           }
        else {
        for(j=0;j<4;j++)
           {
           i=(j+turn)&3;
           if (!(q[i].flags & SD_TRANSPARENT)||(q[i].flags & SD_SECRET)) curcolor=line1;
           else if ((q[i].flags & SD_PLAY_IMPS) && (
                   true_seeing || (q[i].flags & SD_TRUESEE) == 0)) {
               int nx = ss->step_next[i];
               if (nx && !true_seeing) {
                   if (map_sides[(nx*4)+((j+2)&3)].flags & SD_TRUESEE) {
                       continue;
                   }
               }
               curcolor=line2;
           }
           else curcolor=AUTOMAP_FORE;
           if (q[i].flags & SD_INVIS) curcolor=AUTOMAP_FORE;
           if (curcolor!=AUTOMAP_FORE)
              {
              switch (j)
                 {
                 case 0:hor_line32(x,y,x+8);break;
                 case 1:ver_line32(x+8,y,y+8);break;
                 case 2:hor_line32(x,y+8,x+8);break;
                 case 3:ver_line32(x,y,y+8);break;
                 }
              }
           }
        }

  }

void herni_cas(char *s)
  {
  int mes,den,hod,min;
  int32_t cas;

  cas=game_time;
  mes=cas/(360*24*30);cas%=360*24*30;
  den=cas/(360*24);cas%=360*24;
  hod=cas/(360);cas%=360;
  min=cas/6;

  if (mes)
     {
     sprintf(s,texty[cislovka(mes)+149],mes);
     strcat(s," ");
     s=strchr(s,0);
     }
  if (den)
     {
     sprintf(s,texty[cislovka(den)+146],den);
     strcat(s," ");
     s=strchr(s,0);
     }
  sprintf(s,texty[152],hod,min);
  }

static void zobraz_herni_cas(void)
  {
  static char text[100];
  static int32_t old_time=-1;
  char cas[100];

  if (old_time!=game_time)
     {
     herni_cas(cas);
     strcpy(text,texty[145]);
     strcat(text," ");
     strcat(text,cas);
     old_time=game_time;
     }
  set_font(H_FONT6,AUTOMAP_FONT_COLOR);
  set_aligned_position(635,372,2,2,text);
  outtext(text);
  }


extern word color_butt_on[];
extern word color_butt_off[];

static void displ_button(char disable,char **text)
  {
  int posy[]={0,18,37,55};
  int sizy[]={18,20,20,21};
  int i;

  int32_t scr_linelen2 = GetScreenPitch();
  cur_disables=disable;
  set_font(H_FTINY,0);
  put_picture(LEFT,BOTT,ablock(H_CHARGEN));
  for(i=0;i<4;i++)
     {
     if (disable & 1)
        {
        put_8bit_clipped(ablock(H_CHARGENB),(392+posy[i])*scr_linelen2+524+GetScreenAdr(),posy[i],96,sizy[i]);
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


void draw_automap(int xr,int yr)
{
  int i,k,x,y,xp,yp;
  int depth;
//  TSTENA *q;
  //word *s;

  update_mysky();
  schovej_mysku();
  put_textured_bar(ablock(H_BACKMAP),0,17,640,360,-xr*8,-yr*8);
  curcolor=AUTOMAP_BACK;
  xp=map_coord[viewsector].x*8;
  yp=map_coord[viewsector].y*8;
  depth=cur_depth;
  map_xr=xp-xr*8;
  map_yr=yp-yr*8;
  for(k=0;k<2;k++)
  for(i=1;i<mapsize;i++)
  {
    int flagmask=MC_AUTOMAP|(k!=0?MC_DISCLOSED:0);
     if ((map_coord[i].flags & flagmask) &&  (map_coord[i].flags & MC_MARKED) && map_coord[i].layer==depth)
     {
     x=(map_coord[i].x*8+xr*8);
     y=(map_coord[i].y*8+yr*8);
//     q=map_sides+(i*4);
//     s=map_sectors[i].step_next;
     x-=xp;y-=yp;
     if (y>=-178 && y<170 && x>=-312 && x<310)
        {

        x+=320;y+=197;
        draw_amap_sector(x,y,i,k,0,AUTOMAP_LINE1,AUTOMAP_LINE2);
           if (k == 1 && (map_coord[i].flags & MC_PLAYER))
              {
              int j,l=-1;

                        for (j = 0; j < POCET_POSTAV; j++) {
                            if (postavy[j].used && abs(postavy[j].sektor) == i
                                    && postavy[j].inmaphash
                                            == current_map_hash) {
                                if (postavy[j].groupnum == cur_group)
                                    break;
                                l = j;
                            }
                        }
                        if (j == POCET_POSTAV)
                            j = l;
                        if (j != -1) {
                            char c[2];

                            position(x + 1, y + 1);
                            set_font(H_FSYMB,
                                    postavy[j].groupnum == cur_group ?
                                            RGB888(255, 255, 255) :
                                            barvy_skupin[postavy[j].groupnum]);
                            c[0] = postavy[j].direction + 4;
                            c[1] = 0;
                            outtext(c);
                        }
                    }
        }
     }
  }
  ukaz_vsechny_texty_v_mape();
  zobraz_herni_cas();
     {
     char s[50];
     sprintf(s,texty[153],mglob.mapname,depth);
     set_aligned_position(5,372,0,2,s);
     outtext(s);
     }
  ukaz_mysku();
  showview(0,16,640,360);
}
void map_keyboard(EVENT_MSG *msg,void **usr);

void enable_all_map(void)
  {
  int i;
  for(i=1;i<mapsize;i++) map_coord[i].flags|=MC_MARKED;
  }

void disable_all_map(void)
  {
  int i;
  for(i=1;i<mapsize;i++) map_coord[i].flags&=~MC_MARKED;
  }


void unwire_automap(void)
              {
              send_message(E_DONE,E_KEYBOARD,map_keyboard);
              send_message(E_DONE,E_AUTOMAP_REDRAW,map_keyboard);
              hold_timer(TM_FAST_TIMER,0);
              disable_all_map();
              set_select_mode(0);
              pick_set_cursor();
			  GlobEvent(MAGLOB_AFTERMAPOPEN,viewsector,viewdir);
              }



void map_keyboard(EVENT_MSG *msg,void **usr)
  {
  char c;
  static int xr,yr;

  usr;
  if (msg->msg==E_INIT) {
      xr=va_arg(msg->data,int);
      yr=va_arg(msg->data,int);
  }
  if (msg->msg==E_KEYBOARD)
     {
      int d = quit_request_as_escape(va_arg(msg->data, int));
     c=d>>8;
     switch (c)
        {
        case 17:
        case 'H':yr--;break;
        case 31:
        case 'P':yr++;break;
        case 32:
        case 'M':xr++;break;
        case 30:
        case 'K':xr--;break;
        case 16:
        case 'Q':
        case 's':if (check_for_layer(cur_depth-1)) cur_depth--;break;
        case 18:
        case 'I':
        case 'O':
        case 't':if (check_for_layer(cur_depth+1)) cur_depth++;break;
        case 15:
        case 50:
        case 1:
              unwire_proc();
              wire_proc();
              return;
         }
     }
  draw_automap(xr,yr);

  return;
  }

typedef struct  {
    int left,  top,  right,  bottom;
} TMAP_RECT;

static TMAP_RECT get_map_rect() {
    TMAP_RECT rc = {};
    char first = 1;
    for(int i=1;i<mapsize;i++) {
         if ((map_coord[i].flags & (MC_AUTOMAP|MC_DISCLOSED)) &&  (map_coord[i].flags & MC_MARKED)) {
             int xp=map_coord[i].x;
             int yp=map_coord[i].y;
             if (first) {
                 rc.left=rc.right = xp;
                 rc.top=rc.bottom = yp;
                 first = 0;
             } else {
                 if (xp < rc.left) rc.left = xp;
                 if (yp < rc.top) rc.top = yp;
                 if (xp > rc.right) rc.right = xp;
                 if (yp > rc.bottom) rc.bottom = yp;
             }
         }
    }
    return rc;
}

static TMAP_RECT get_screen_rect() {
    TMAP_RECT rc;
    rc.left = -35;
    rc.right = 35;
    rc.top = -18;
    rc.bottom = 18;
    return rc;
}

static void shift_map_to_center(TMAP_RECT map_rect, int mx, int my, int *xr, int *yr) {
    TMAP_RECT screen_rect = get_screen_rect();
    int dx = (map_rect.left+map_rect.right)/2-mx;  //
    int dy = (map_rect.top+map_rect.bottom)/2-my;
    if (dx < screen_rect.left) dx = screen_rect.left;
    if (dx > screen_rect.right) dx = screen_rect.right;
    if (dy < screen_rect.top) dy = screen_rect.top;
    if (dy > screen_rect.bottom) dy = screen_rect.bottom;;
    *xr = -dx;
    *yr = -dy;
}

void show_automap(char full)
  {
  mute_all_tracks(0);
  unwire_proc();
  if (full) enable_all_map();
  hold_timer(TM_FAST_TIMER,1);
  unwire_proc=unwire_automap;
  schovej_mysku();
  if (cur_mode!=MD_ANOTHER_MAP) bott_draw(1),cur_mode=MD_MAP;
  other_draw();
  if (!battle && full && enable_glmap) displ_button(cur_mode==MD_ANOTHER_MAP?4:6,texty+210);
  else displ_button(7,texty+210);
  ukaz_mysku();
  showview(0,376,640,480);
  cur_depth=map_coord[viewsector].layer;
  int xm = map_coord[viewsector].x;
  int ym = map_coord[viewsector].y;
  int xr;
  int yr;
  shift_map_to_center(get_map_rect(), xm, ym, &xr, &yr);
  draw_automap(xr,yr);

  send_message(E_ADD,E_KEYBOARD,map_keyboard,xr,yr);
  send_message(E_ADD,E_AUTOMAP_REDRAW,map_keyboard,xr,yr);
  change_click_map(clk_map_view,CLK_MAP_VIEW);
}

static char mob_not_invis(int sector)
  {
  int m;
  m=mob_map[sector];
  while (m)
     {
     m--;if (mobs[m].vlastnosti[VLS_KOUZLA] & SPL_INVIS) return 0;
     m=mobs[m].next;
     }
  return 1;
  }

void draw_medium_map(void)
  {
  int xr, yr;
  int xp, yp;
  int xc=0,yc=0,x=0,y=0;
  int j,i,k,layer;
  //char c=" ";

  xp=MEDIUM_MMAP*8+5;
  yp=MEDIUM_MMAP*8+20;
  layer=map_coord[viewsector].layer;
  xr=map_coord[viewsector].x;
  yr=map_coord[viewsector].y;
  trans_bar(0,17,MEDIUM_MAP*8+6*2,MEDIUM_MAP*8+4*2,0);
  for(j=0;j<2;j++)
     for(i=1;i<mapsize;i++)
        if (map_coord[i].flags & 1 && map_coord[i].layer==layer)
           {
           switch (viewdir & 3)
              {
              case 0:xc=map_coord[i].x-xr;yc=map_coord[i].y-yr;break;
              case 1:yc=-map_coord[i].x+xr;xc=map_coord[i].y-yr;break;
              case 2:xc=-map_coord[i].x+xr;yc=-map_coord[i].y+yr;break;
              case 3:yc=map_coord[i].x-xr;xc=-map_coord[i].y+yr;break;
              }
           if (xc>=-MEDIUM_MMAP && yc>=-MEDIUM_MMAP && yc<=MEDIUM_MMAP && xc<=MEDIUM_MMAP)
              {
              draw_amap_sector(x=xc*8+xp,y=yc*8+yp,i,j,viewdir &3,MEDIUM_MAP_LINE1,MEDIUM_MAP_LINE2);
              if (j)
              if (mob_map[i] && mob_not_invis(i) && battle)
                 {
                 position(x+1,y+1);set_font(H_FSYMB,AUTOMAP_MOB);
                 outtext("N");
                 }
              if (map_coord[i].flags & MC_PLAYER)
                 {
                 int u=-1,z=-1;
                 for(k=0;k<POCET_POSTAV;k++)
                    if (postavy[k].sektor==i) {
                       if (postavy[k].groupnum==cur_group) z=k;else u=k;
                    }
                 if (z!=-1) u=z;
                 if (u!=-1)
                    {
                    set_font(H_FSYMB,postavy[u].groupnum==cur_group && !battle?RGB888(255,255,255):barvy_skupin[postavy[u].groupnum]);
                    position(x+1,y+1);
                    outtext("M");
                    }

                 }
              }
           }
  }

char map_menu_glob_map(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya,xr,yr;
  id=set_select_mode(0);
  if (id)
     {
     schovej_mysku();
     pick_set_cursor();
     displ_button(1,texty+210);
     ukaz_mysku();
     showview(520,378,120,120);
     return 1;
     }
  return 0;
  }


static void wire_glob_map_control(void)
  {
  set_select_mode(0);
  schovej_mysku();
  other_draw();
  displ_button(1,texty+210);
  wire_global_map();
  ukaz_mysku();
  update_mysky();
  change_click_map(clk_glob_map,CLK_GLOB_MAP);
  }


char map_menu(int id,int xa,int ya,int xr,int yr)
  {
  const char *s;
  const word *c;

  ya;xa;id;
  id=set_select_mode(0);
  s=ablock(H_CHARGENM);
  c=(const word *)s;
  s+=*c*yr+xr+6;
  id=*s;
  if (!id) return 1;
  if (id>4) return 1;
  id--;
  if (cur_disables & (1<<id)) return 1;
  if (id)
     {
     pick_set_cursor();
     displ_button(1,texty+210);
     }
  if (cur_mode==MD_ANOTHER_MAP) {unwire_proc();wire_proc();}
  switch (id)
     {
     case 0:wire_glob_map_control();break;
     case 1:unwire_proc();show_automap(1);break;
     case 2:set_select_mode(1);
            schovej_mysku();
            displ_button(5,texty+210);
            mouse_set_default(H_MS_WHO);
            ukaz_mysku();
            showview(0,0,0,0);break;
     case 3:
	   unwire_proc();wire_proc();break;
     }
  return 1;
  }


char exit_kniha(int id,int xa,int ya, int xr, int yr)
  {
  xa;ya;xr;yr;id;
  unwire_proc();
  wire_proc();
  return 1;
  }

int selected_page;


char page_change(int id,int xa,int ya,int xr,int yr)
  {
  int oldp;
  xa,ya,xr,yr;

  oldp=cur_page;
  /*
  if (*(char *)0x417 & 0x3)
     {
     selected_page=cur_page+id;
     mouse_set_default(H_MS_LIST);
     mouse_set_cursor(H_MS_LIST);
     return 1;
     }
   */
  xa=count_pages();
  xa=((xa-1) & ~1)+1;
  cur_page+=id;
  if (cur_page<1) cur_page=1;
  if (cur_page>xa) cur_page=xa;
  if (cur_page!=oldp) play_sample_at_channel(H_SND_KNIHA,1,100);
  wire_kniha();
  return 1;
  }

#define CLK_KNIHA 3
T_CLK_MAP clk_kniha[]=
  {
  {2,320,0,639,479,page_change,2,-1},
  {-2,0,0,319,479,page_change,2,-1},
  {-1,0,0,639,479,exit_kniha,8,-1},
  };


static void kniha_keyboard_proc(EVENT_MSG *msg, void **_) {
    if (msg->msg == E_KEYBOARD) {
        int c = quit_request_as_escape(va_arg(msg->data,int));
        switch(c>>8) {
            case 1: exit_kniha(1, 0, 0, 0, 0);break;
            case 0x4d: page_change(2, 0, 0, 0, 0);break;
            case 0x4b: page_change(-2, 0, 0, 0, 0);break;
            default:break;
        }
    }
}

void unwire_kniha(void)
{
  hold_timer(TM_FAST_TIMER,0);
  GlobEvent(MAGLOB_AFTERBOOK,viewsector,viewdir);
  send_message(E_DONE,E_KEYBOARD, kniha_keyboard_proc);
}


void wire_kniha(void)
  {
  int xa;
  if (!GlobEvent(MAGLOB_BEFOREBOOK,viewsector,viewdir))
  {
    return;
  }
  xa=count_pages();
  xa=((xa-1) & ~1)+1;
  if (cur_page<1) cur_page=1;
  if (cur_page>xa) cur_page=xa;
  mute_all_tracks(0);
  unwire_proc();
  schovej_mysku();
  put_picture(0,0,ablock(H_KNIHA));
  change_click_map(clk_kniha,CLK_KNIHA);
  send_message(E_ADD,E_KEYBOARD, kniha_keyboard_proc);
  unwire_proc=unwire_kniha;
  set_font(H_FONT6,AUTOMAP_FONT_COLOR);
  write_book(cur_page);
  ukaz_mysku();
  showview(0,0,0,0);
  hold_timer(TM_FAST_TIMER,1);
  }

static int last_selected;

char map_target_cancel(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya,xr,yr;
  return exit_wait=1;
  }

void map_teleport_keyboard(EVENT_MSG *msg,void **usr)
  {
  usr;
  if (msg->msg==E_KEYBOARD) {
      int c = quit_request_as_escape(va_arg(msg->data, int));
     switch (c>>8)
        {
        case 1:
        case 15:
        case 50: exit_wait=1; msg->msg=-1;break;
        }
  }
  }


static char path_ok(word sector, void *ctx)
  {
  map_coord[sector].flags|=MC_MARKED;
  return 1;
  }

char map_target_select(int id,int xa,int ya,int xr,int yr)
  {
  int x1,y1,x2,y2;

  ya,xa;
  for (id=1;id<mapsize;id++)
     if ((map_coord[id].flags & (MC_AUTOMAP|MC_DISCLOSED)) && (map_coord[id].flags & (MC_MARKED)) && map_coord[id].layer == cur_depth)
     {
     x1=(map_coord[id].x*8-map_xr);
     y1=(map_coord[id].y*8-map_yr);
     x1+=320;
     y1+=197-SCREEN_OFFLINE;
     x2=x1+8;
     y2=y1+8;
     if (xr>=x1 && xr<=x2 && yr>=y1 && yr<=y2)
        {
        last_selected=id;
        exit_wait=1;
        return 1;
        }
     }
  return 0;
  }


int select_teleport_target(char nolimit)
  {
  *otevri_zavoru=1;
  unwire_proc();
  disable_all_map();
  if (nolimit) {
      enable_all_map();
  } else {
      labyrinth_find_path(viewsector,65535,SD_PLAY_IMPS,path_ok,NULL,NULL);
      map_coord[viewsector].flags|=MC_MARKED;
  }
  schovej_mysku();
  send_message(E_ADD,E_KEYBOARD,map_teleport_keyboard);
  show_automap(0);
  change_click_map(clk_teleport_view,CLK_TELEPORT_VIEW);
  last_selected=0;
  ukaz_mysku();
  escape();
  send_message(E_DONE,E_KEYBOARD,map_teleport_keyboard);
  disable_all_map();
  return last_selected;
  }


void save_map_description(TMPFILE_WR *f)
  {
  int count,max;
  int32_t i;

  if (texty_v_mape==NULL) max=0;else max=str_count(texty_v_mape);
  for(i=0,count=0;i<max;i++) if (texty_v_mape[i]!=NULL) count++;
  temp_storage_write(&count,1*sizeof(count),f);
  for(i=0;i<max;i++) if (texty_v_mape[i]!=NULL)
     {
     word len;
     len=strlen(texty_v_mape[i]+12)+12+1;
     temp_storage_write(&len,1*2,f);
     temp_storage_write(texty_v_mape[i],1*len,f);
     }
  }

void load_map_description(TMPFILE_RD *f)
  {
  int32_t count;
  int32_t i;
  word len;

  if (texty_v_mape!=NULL)release_list(texty_v_mape);
  temp_storage_read(&count,1*sizeof(count),f);
  if (!count)
     {
     texty_v_mape=NULL;
     return;
     }
  texty_v_mape=create_list(count);
  for(i=0;i<count;i++)
     {
      temp_storage_read(&len,1*2,f);
        {
        char *s;
        s=(char *)alloca(len);
        memset(s,1,len-1);
        s[len-1]=0;
        str_replace(&texty_v_mape,i,s);
        }
        temp_storage_read(texty_v_mape[i],1*len,f);
     }
  }

void free_map_description() {
    if (texty_v_mape!=NULL)release_list(texty_v_mape);
    texty_v_mape = NULL;
}

TSTR_LIST swap_map_description(TSTR_LIST new_list) {
   TSTR_LIST old = texty_v_mape;
   texty_v_mape = new_list;
   return old;
}