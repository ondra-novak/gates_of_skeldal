//Toto je hlavni soubor specialnich procedur pro hru BRANY SKELDALU
#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <libs/event.h>
#include <libs/strlite.h>
#include <libs/bgraph.h>
#include "globals.h"
#include "specproc.h"
#include <libs/bmouse.h>
#include <math.h>
//#include "i86.h"

#define MOB_GO(m) if (m->dir & 1)m->headx=mob_go_x[m->dir];else m->heady=mob_go_y[m->dir]

int cur_event_number;

static void event_error(char *text,int number)
  {
  char buff[256];
  sprintf(buff,"%s\n Specproc num: %d\n",text,number);
  display_error(buff);
  exit(1);
  }


//Item procs---------------------------------------
ITEM_PROC(item_test)
  {
  putchar('\x7');event,ptr,p;
  putchar('\n');
  return 1;
  }

//Map procs---------------------------------------
MAP_PROC(map_test)
  {
  putchar('\x7');sector,side,event,value;
  putchar('\n');
  return 1;
  }


MAP_PROC(map_teleport)
  {
  side=0;event;
  teleport_target=value;
  if (mob_map[sector]!=0)
     {
     int i;
     spell_teleport(-mob_map[sector],-1,-1);
     if ((i=mobs[mob_map[sector]-1].next)!=0)
        spell_teleport(-i,-1,-1);
     side=1;
     }
  if (map_coord[sector].flags & MC_DPLAYER)
     {
     int i,j=-1;THUMAN *h;
     int bit=0;

     for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->used && h->sektor==sector && h->sektor==viewsector && h->inmaphash == current_map_hash)
        {
        j=i;break;
        }
     for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->used && h->sektor==sector && h->inmaphash == current_map_hash) bit|=1<<i;
     postavy_teleport_effect(teleport_target,h->direction,bit,j!=-1);
     side=1;
     auto_group();
     }
  return side;
  }

/*
static void otoc_obraz1(word *source,word *target)
  {
  word *p,*q;
  word *sp,*sq;
  int x,y;

  sp=source+320-180;
  sq=target+scr_linelen2*359+320-180;
  y=360;
  while (y--)
     {
     p=sp;
     q=sq;
     x=360;
     while (x--)
        {
        *q=*p;
        q-=scr_linelen2;
        p++;
        }
     sp+=scr_linelen2;
     sq++;
     }
  }

void swap_screen(word *_p,word *_q)
  {
  __asm
    {
    mov esi,_p
    mov edi,_q

    std
    mov  ecx,115200
    lp1:
    mov  ax,[edi]
    xchg ax,[esi]
    stosw
    add  esi,2
    dec  ecx
    jnz  lp1
    cld
    }
  }
//  #pragma aux swap_screen parm [esi][edi]=\ modify [ecx eax]

static void otoc_obraz2(word *source)
  {
  swap_screen(source,source+scr_linelen2*360-2);
  }

static void otoc_obraz3(word *source,word *target,short smer)
  {
  word *tt,*ss;
  int x,y,xs,ys;
  int xp,yp;

  tt=target;
  if (smer>0)x=320+126-224;else x=320+126+224;
  if (smer>0)y=180-126-224;else y=180+126-224;
  yp=360;
  ys=0;
  while (yp--)
     {
     xs=0;
     ss=source+scr_linelen2*y+x;
     xp=640;
     while (xp--)
        {
        if (ss>=source && ss<source+scr_linelen2*360) *tt++=*ss; else *tt++=0;
        xs+=7;
        if (xs>10)
           {
           xs-=10;
           ss+=scr_linelen2+smer;
           }
        }
     ys+=7;
     if (ys>10)
        if (smer>0)
        {
        ys-=10;
        x--;
        y++;
        }
        else
        {
        ys-=10;
        x--;
        y--;
        }
     }
  }

static void show_liane(THE_TIMER *t)
  {
  word *bt,*bs;
  static int counter=0;

  schovej_mysku();
  if (counter<5 && counter)
     {
     bt=GetScreenAdr()+SCREEN_OFFSET;
     bs=GetBuffer2nd()+SCREEN_OFFSET;
     }
     else
     {
     redraw_scene();
     bs=GetScreenAdr()+SCREEN_OFFSET;
     }

  switch(counter)
     {
     case 0:break;
     case 1:otoc_obraz3(bs,bt,1);break;
     case 2:curcolor=0;
            bar(0,SCREEN_OFFLINE,139,SCREEN_OFFLINE+359);
            bar(640-140,SCREEN_OFFLINE,639,SCREEN_OFFLINE+359);
            otoc_obraz1(bs,bt);break;
     case 3:otoc_obraz3(bs,bt,-1);break;

     default:otoc_obraz2(bs);if (counter==4)OutBuffer2nd();break;
     }
  ukaz_mysku();
  showview(0,0,0,0);
  if (!counter)
     {
     schovej_mysku();
     OutBuffer2nd();
     ukaz_mysku();
     }
  if (t->calls==1)
     {
     THUMAN *h;
     int i;

     save_load_trigger((word)(t->userdata[0]));
     for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (get_player_triggered(i))
        {
        player_hit(h,h->lives,0);
        h->sektor=0;
        }
     wire_proc();
     counter=-1;
     }
  counter++;
  }
*/




static void OtocObrazPodleMatice(float mx[3][2], word *picture)
  {
    int32_t scr_linelen2 = GetScreenPitch();

  word *trg=GetScreenAdr()+17*scr_linelen2;
  int x,y;
  picture+=6;
  for (y=0;y<360;y++,trg+=scr_linelen2)
	for (x=0;x<640;x++)
	  {
	  int oldx=x-320;
	  int oldy=y-180;
	  int newx=(int)(oldx*mx[0][0]+oldy*mx[1][0]+320);
	  int newy=(int)(oldx*mx[0][1]+oldy*mx[1][1]+180);
	  if (newx>=0 && newx<640 && newy>=0 && newy<360)
		trg[x]=picture[newx+640*newy];
	  else
		trg[x]=0;
	  }
  }

static void OtaceniObrazu()
  {
  word *picture=(word *)malloc(640*360*2+16);
  float mx[3][2];

  int maxtime=500;
  int lasttime=get_game_tick_count();
  int curtime;
  get_picture(0,17,640,360,picture);
  do
	{
	float phase;
	float uhel;
	float cosuhel;
	float sinuhel;
	curtime=get_game_tick_count()-lasttime;
	phase=curtime/(float)maxtime;
	if (phase>1.0f) phase=1.0f;
	uhel=phase*3.14159265;
	cosuhel=cos(uhel);
	sinuhel=sin(uhel);
	mx[0][0]=cosuhel;
	mx[0][1]=sinuhel;
	mx[1][0]=-sinuhel;
	mx[1][1]=cosuhel;
	OtocObrazPodleMatice(mx,picture);
	showview(0,0,0,0);
	do_events();
	}
  while (curtime<maxtime);
  sleep_ms(1000);
  free(picture);
  }

MAP_PROC(map_liana)
  {
  int i;
  THUMAN *h;

  value;
  sector,side,event;
  bott_draw(1);
  other_draw();
  unwire_proc();
//  add_to_timer(TM_SCENE,gamespeed,15,show_liane)->userdata[0]=save_load_trigger(-1);
  schovej_mysku();
  OtaceniObrazu();
  ukaz_mysku();
     for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (get_player_triggered(i))
        {
        player_hit(h,h->lives,0);
        h->sektor=0;
        }
  redraw_scene();
  cancel_pass=1;
  showview(0,0,0,0);
  wire_proc();
  return 1;
  }
#define ID_XS 400
#define ID_YS 380

MAP_PROC(map_identify)
  {
  int x,y,yp,xp,ys;
  int i,cnt;char s[100];
  TITEM *it;
  TSTR_LIST ls;

  sector;side;event;value;
  if (picked_item==NULL) return 0;
  it=glob_items+*picked_item-1;
  ls=create_list(256);
  unwire_proc();
  sprintf(s,texty[210],it->jmeno);str_add(&ls,s);
  sprintf(s,texty[211],it->hmotnost*2,it->hmotnost>0 && it->hmotnost<3?texty[236]:texty[237]);str_add(&ls,s);
  if (it->nosnost)
     {
     sprintf(s,texty[212],it->nosnost);str_add(&ls,s);
     }
  for(i=0;i<21;i++)
     if (it->zmeny[i] && texty[213+i]!=NULL)
        {
        if (i==VLS_HPREG || i==VLS_MPREG || i==VLS_VPREG)
           sprintf(s,texty[213+i],it->zmeny[i]>0?texty[234]:texty[235]);
        else
           sprintf(s,texty[213+i],it->zmeny[i],it->zmeny[i+1]);
        str_add(&ls,s);
        }
  if (it->zmeny[VLS_MGSIL_H])
     {
     sprintf(s,texty[233],texty[22+it->zmeny[VLS_MGZIVEL]]);str_add(&ls,s);
     }
  for(i=0;i<16;i++)
     if (it->zmeny[VLS_KOUZLA] & (1<<i) && texty[i+240]!=NULL) str_add(&ls,texty[i+240]);
  for(i=str_count(ls);i>0;i--) if (ls[i-1]!=NULL) break;
  cnt=i;i=0;
  ys=cnt*10+10;
  do
     {
     x=320-ID_XS/2;
     y=240-ys/2;
     create_frame(x,y,ID_XS,ys,1);
     xp=x+5;yp=y+5;ys=ID_YS-10;
     set_font(H_FBOLD,NOSHADOW(0));
     while(i<cnt)
        {
        position(xp,yp);outtext(ls[i]);
        yp+=10;
        i++;
        }
     showview(0,0,0,0);
     wire_proc();
     }
  while (i<cnt);
  return 1;
  }

//Mob Procs ---------------------------------------

static int get_dangerous_place(int sector)
  {
  int i;

  for(i=0;i<4;i++)
     {
     int s=map_sectors[sector].step_next[i];
     if (s && map_coord[s].flags & MC_PLAYER) return 1;
     }
  return 0;
  }

MOB_PROC(mob_open_door)
  {
  if (event==SMPR_WALK) {
     if (m->user_data<128 || m->user_data>192)
        {
        int sector=m->sector;
        int i;
        TSTENA *side;

        for(i=0,side=map_sides+(sector<<2);i<4;side++,i++)
           if (side->flags & SD_MONST_IMPS && side->sector_tag!=0 && (~side->flags & (SD_PASS_ACTION | SD_SECRET))==(SD_PASS_ACTION|SD_SECRET))
              break;
        if (i!=4)
           {
           m->dir=i;stop_mob(m);
           if (flag_map[(sector<<2)+i] & SD_MONST_IMPS) a_touch(sector,m->dir);
           m->user_data=128;
           return 1;
           }
        side=map_sides+(sector<<2)+((m->dir+2)&3);
        if (~side->flags & SD_MONST_IMPS && side->sector_tag!=0 && (~side->flags & (SD_PASS_ACTION | SD_SECRET))==(SD_PASS_ACTION|SD_SECRET))
           {
           int ss=map_sectors[sector].step_next[(m->dir+2)&3];
           int j=mob_map[ss];
           while (j) if (mobs[j-1].dir==m->dir) return 0;
              else j=mobs[j-1].next;
           a_touch(sector,(m->dir+2)&3);
           return 0;
           }
        }
     else
        {
        if (m->user_data>=128 && m->user_data<192) m->user_data++;
        if (~map_sides[(m->sector<<2)+m->dir].flags & SD_MONST_IMPS)
           {
           if (m->dir & 1) m->headx=mob_go_x[m->dir];else  m->heady=mob_go_y[m->dir];
           m->user_data=map_coord[map_sectors[m->sector].step_next[m->dir]].flags & MC_PLAYER?255:127;
           }
        return m->user_data<144;
        }
  }
  return 0;
  }

MOB_PROC(mob_open_door_battle)
  {
  if (event==SMPR_ATTACK)
     {
     m->specproc=5;
     }
  return 0;
  }

static char spec_proc_test_mob(int event_type,TMOB *m)
  {
  m,event_type;
  putchar('\x7');
  putchar('\n');
  return 0;
  }


static char mob_dokola(int event_type,TMOB *m)
  {
  if (event_type==SMPR_WALK)
     {
     m->dir++;
     m->dir &=3;
     MOB_GO(m);
     return 1;
     }
  return 0;
  }

static char mob_carodej(int event_type,TMOB *m)
  {
static char kouzla[5]={3,8,13,18,80};

  if (event_type==SMPR_ATTACK)
     {
     m->casting=kouzla[rnd(5)];
     }
  return 0;
  }

static char mob_strelec(int event_type,TMOB *m)
  {
  if (event_type==SMPR_ATTACK)
     {
     int i,l;

     if (m->dostal==0 && ~m->user_data & 128) return 0;
     for (i=0;i<4;i++)
        {
        l=map_sectors[m->sector].step_next[i];
        if (l!=0 && map_coord[l].flags & MC_PLAYER) break;
        }
     if (i==4)
        {
        if (m->user_data & 128)
           {
           int s=m->sector;
           m->user_data&=~128;
           i=(m->dir+2)&3;
           while (s && !(map_coord[s].flags & MC_PLAYER)) if (map_sides[(s<<2)+i].flags & SD_MONST_IMPS) s=0;else s=map_sectors[s].step_next[i];
           if (s) m->dir=i;else return 1;
           }
        return 0; //strilej
        }
     i=(i+2)&3;
     if (mob_check_next_sector(m->sector,i,m->stay_strategy & MOB_BIG,0))
        {
        int l=4,z,max=RAND_MAX;
        for(i=0;i<4;i++)
           if (!mob_check_next_sector(m->sector,i,m->stay_strategy & MOB_BIG,0))
              {
              int s=map_sectors[m->sector].step_next[i];
              if (!get_dangerous_place(s) && (z=rand())<max) max=z,l=i;
              }
        if (l==4) return 0;
        i=l;
        }
     m->dir=i;
     MOB_GO(m);
     m->user_data|=128;
     return 1;
     }
  else
     {
     /*
     int i,l;
        i=m->dir;
        l=m->sector;
        if (map_sides[(l<<2)+i].flags & SD_MONST_IMPS) return 0;
        l=map_sectors[l].step_next[i];
        for(i=0;i<4;i++)
           {
           int s=map_sectors[l].step_next[i];
           if (isplayer(s,NULL,0)!=NULL)
              {
              m->dir=i+2&3;
              m->headx=mob_go_x[i];
              m->heady=mob_go_y[i];
              return 1;
              }
           }
      */
      if (m->user_data & 128)
        {
        THUMAN *h;
        int i;

        for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
           if (h->lives && h->used  && h->inmaphash == current_map_hash&&
               abs(map_coord[h->sektor].x-map_coord[m->sector].x)<2 &&
               abs(map_coord[h->sektor].y-map_coord[m->sector].y)<2)
                 {
                 stop_mob(m);
                 return 1;
                 }
        }
      return 0; //delej si co chces
      }
  }




MOB_PROC(mob_krikloun)
  {
  if (event==SMPR_ATTACK)
     {
     sirit_zvuk(m->sector);
     }
  return 0;
  }

static char mob_taktik_recurse(int recall,int sector,int *min_obr,int big,int *csect,int *cdir)
  {
     int i;
     char nasel=0;

     //nejprve zjistime kdo je okolo nas
     for(i=0;i<4;i++)
        {
        int s=map_sectors[sector].step_next[i];
        THUMAN *h=NULL;

        if (!s) continue; //pokud tam je stena tak pokracuj jinym smerem
        if (mob_check_next_sector(sector,i,big,0)==1) continue;
        if ((h=isplayer(s,h,0))!=NULL) //nekdo tam je - zjisti kdo
           while(h!=NULL)
           {
           if (h->vlastnosti[VLS_OBRAN_H]<*min_obr) // pokud ma nizsi obranu
               {
               *csect=sector;
               *cdir=i;
               *min_obr=h->vlastnosti[VLS_OBRAN_H];
               nasel=1;
               }
           h=isplayer(s,h,0);
           }
        else if (recall) //pokud tam nikdo neni koukni se vedle
           {
           if (mob_taktik_recurse(0,s,min_obr,big,csect,cdir)) *cdir=i;
           }
        }
   return nasel;
  }


MOB_PROC(mob_taktik)
  {
  int min_obr=1000;
  int csect=m->sector;
  int cdir=-1;

  if (event==SMPR_ATTACK)
     {
     mob_taktik_recurse(1,m->sector,&min_obr,m->stay_strategy & MOB_BIG,&csect,&cdir);
     if (cdir==-1) return 0;
     m->dir=cdir;
     if (m->sector!=csect)
        {
        MOB_GO(m);
        return 1;
        }

     }
  return 0;
  }

MOB_PROC(mob_stoji)
  {
  if (event==SMPR_ATTACK)
     {
     int i;
     int s;

     for(i=0;i<4;i++)
        {
        s=map_sectors[m->sector].step_next[i];
        if (s && map_coord[s].flags & MC_PLAYER) return 0;
        }
     m->headx=m->locx;
     m->heady=m->locy;
     m->stay_strategy&=~(MOB_WALK | MOB_LISTEN);
     m->vlajky&=~MOB_IN_BATTLE;
     return 1;
     }
  else if (event==SMPR_WALK)
     {
     m->headx=m->locx;
     m->heady=m->locy;
     }
  else if (event==SMPR_KNOCK) return 1;
  return 0;
  }

static t_mob_proc sp_mob_table[]=
  {
  NULL,                   //0
  spec_proc_test_mob,     //1
  mob_dokola,             //2
  mob_carodej,            //3
  mob_strelec,            //4
  mob_open_door,          //5
  mob_open_door_battle,   //6
  mob_krikloun,           //7
  mob_taktik,             //8
  mob_stoji,              //9
  };

#define SP_MOB_TAB_SIZE (sizeof(sp_mob_table)/sizeof(t_mob_proc))

static t_item_proc sp_item_table[]=
  {
  NULL,                   //0
  item_test,              //1
  };

#define SP_ITEM_TAB_SIZE (sizeof(sp_item_table)/sizeof(t_item_proc))

static t_map_proc sp_map_table[]=
  {
  NULL,                   //0
  map_test,               //1
  map_teleport,           //2
  map_liana,              //3
  map_identify,           //4
  };

#define SP_MAP_TAB_SIZE (sizeof(sp_map_table)/sizeof(t_map_proc))

char call_mob_event(int event_number,int event_type,TMOB *m)
  {
  if (!event_number) return 0;
  if (event_number>=(int)SP_MOB_TAB_SIZE)
     event_error("Nestv�ra pou��va neplatnou specproc.",event_number);
  cur_event_number=event_number;
  return sp_mob_table[event_number](event_type,m);
  }

char call_item_event(int event_number,int event_type,short *ptr,THUMAN *p)
  {
  if (!event_number) return 0;
  if (event_number>=(int)SP_ITEM_TAB_SIZE)
     event_error("��slo ud�losti u v�ci je neplatn�. Specproc nen� definov�na.",event_number);
  cur_event_number=event_number;
  return sp_item_table[event_number](event_type,ptr,p);
  }

char call_map_event(int event_number,int sector,int side,int value,int event)
  {
  if (!event_number) return 0;
  if (event_number>=(int)SP_MAP_TAB_SIZE)
     event_error("Neplatn� ��slo ud�losti na st�n�. Specproc s t�mto ��slem nen� definov�na.",event_number);
  cur_event_number=event_number;
  return sp_map_table[event_number](sector,side,value,event);
  }

