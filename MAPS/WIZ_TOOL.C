#include <skeldal_win.h>
#include <dos.h>
#include <types.h>
#include <mem.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <memman.h>
#include <bgraph.h>
#include <event.h>
#include <devices.h>
#include <bmouse.h>
#include <gui.h>
#include <strlite.h>
#include <strlists.h>
#include <inicfg.h>
#include <wav_mem.h>
#include <zvuk.h>
#include "mapy.h"
#include <basicobj.h>
#include <direct.h>
#include "globals.h"
#include "edit_map.h"
#include "steny.h"
#include "mob_edit.h"
#include "save_map.h"

long wiz_tool_numb=-1;
MAPGLOBAL mglob={"","","","",0,0,0,1};

PTRMAP *smplist=NULL;

TMULTI_ACTION **multi_actions[MAPSIZE*4];
int multiaction_win=-1;
void open_multiaction_window();
static TSTR_LIST mul_list=NULL;

void create_isort_list();


int action_sizes[MAX_ACTIONS]=
  {sizeof(TMA_GEN),sizeof(TMA_SOUND),sizeof(TMA_TEXT),sizeof(TMA_TEXT),
   sizeof(TMA_SEND_ACTION),sizeof(TMA_FIREBALL),sizeof(TMA_GEN),sizeof(TMA_LOADLEV),
   sizeof(TMA_DROPITM),sizeof(TMA_TEXT),sizeof(TMA_TEXT),sizeof(TMA_CODELOCK),
   sizeof(TMA_CACTN),sizeof(TMA_LOCK),sizeof(TMA_SWAPSC),sizeof(TMA_WOUND),
   sizeof(TMA_TWOP),sizeof(TMA_TWOP),sizeof(TMA_TWOP),sizeof(TMA_TEXT),sizeof(TMA_TWOP),
   sizeof(TMA_TWOP),sizeof(TMA_TWOP),sizeof(TMA_LOADLEV),sizeof(TMA_DROPITM),
   sizeof(TMA_TWOP),sizeof(TMA_TWOP),sizeof(TMA_UNIQ),sizeof(TMA_TWOP),sizeof(TMA_UNIQ),
   sizeof(TMA_TWOP),sizeof(TMA_LOADLEV),sizeof(TMA_TWOP),sizeof(TMA_LOADLEV),
   sizeof(TMA_TWOP),sizeof(TMA_TWOP),sizeof(TMA_TEXT),sizeof(TMA_GLOBE),sizeof(TMA_IFSEC),sizeof(TMA_TWOP)};

void close_wiz_tool(void)
  {
  WINDOW *p;
  p=find_window(wiz_tool_numb);
  if (p!=NULL) close_window(p);
  }

int count_of_sel(void)
  {
  int i=0,j=1;

  for(;j<maplen;j++)
     if (minfo[j].flags & 1) i++;
  return i;
  }

void remote_acces(void)
  {
  char rem,swp;
  int i;

  rem=!f_get_value(0,90);
  swp=f_get_value(0,200);
  for(i=100;i<150;i+=10)set_enable(0,i,rem || !swp);
  for(i=150;i<200;i+=10)set_enable(0,i,rem || swp);
  set_enable(0,80,rem);
  set_enable(0,210,rem);
  set_enable(0,220,rem);
  }

void build_door(void)
  {
  long flags1=0,flags2=0;
  char remote,swp;
  int side1,side2,anim1,anim2;
  int dir,sector1,sector2;
  int mode,lclip;

  side1=f_get_value(0,20);anim1=vals(30)-1;
  side2=f_get_value(0,50);anim2=vals(60)-1;
  lclip=vals(210);
  if (anim1!=anim2 && (anim1==0 || anim2==0))
     {
     msg_box("Nelze!",'\x1',"Po�et animac� m��e b�t bu� u obou jedna, nebo se ��dn� nesm� jedn� rovnat","Ok",NULL);
     return;
     }
  dir=f_get_value(0,70);
  sector1=f_get_value(0,300);
  flags1=get_bit_fields(0,100,5)^1;
  flags2=get_bit_fields(0,150,5)^1;
  mode=f_get_value(0,80);
  remote=f_get_value(0,90);
  swp=f_get_value(0,200);
  if (dir!=4)
     {
     TSTENA *sd1,*sd2;
     if (!mapa.sectordef[sector1].step_next[dir])
        {
        msg_box("T�mto sm�rem nelze vytvo�it dve�e",'\x1',"Ve v�mi zvolenem sm�ru nen� vytvo�en ��dn� sektor. Nelze t�mto sm�rem vytvo�it dve�e","Ok",NULL);
        return;
        }
     sector2=mapa.sectordef[sector1].step_next[dir];
     sd1=&mapa.sidedef[sector1][dir];
     sd2=&mapa.sidedef[sector2][(dir+2)&3];
     sd1->flags &=~(0x40F1F | SD_LEFT_ARC | SD_RIGHT_ARC);
     sd2->flags &=~(0x40F1F | SD_LEFT_ARC | SD_RIGHT_ARC);
     sd1->lclip=lclip;
     sd2->lclip=lclip;
     if (swp)
        {
        sd1->flags|=flags2 | 0x800;
        sd2->flags|=flags2 | 0x800;
        if (anim1!=0)
           {
           sd1->flags|=0x0200;
           sd2->flags|=0x0200;
           }
        }
     else
        {
        sd1->flags|=flags1;
        sd2->flags|=flags1;
        sd1->flags|=0x0200;
        sd2->flags|=0x0200;
        }
     sd1->prim=side1;sd2->prim=side2;
     sd1->prim_anim=anim1;sd2->prim_anim=anim2;
     if (!remote)
        {
        sd1->flags&=~0xfff00000;
        sd1->flags|=((flags1^flags2)<<24)| 0x00400000;
        sd1->action=1+mode+4*(anim1==0);
        sd1->sector_tag=sector1;
        sd1->side_tag=dir;
        sd2->flags&=~0xfff00000;
        sd2->flags|=((flags1^flags2)<<24)| 0x00400000;
        sd2->action=1+mode+4*(anim1==0);
        sd2->sector_tag=sector1;
        sd2->side_tag=dir;
        }
     }
  else
     {
     word *w,i,k,d;
     TSTENA *sd;

     w=mapa.sectordef[sector1].step_next;
     if (!w[0] && !w[1] && !w[2] && !w[3])
        {
        msg_box("Nejsou ��dn� v�chody:",'\x1',"Zde nelze vytvo�it tento druh st�ny","Ok",NULL);
        return;
        }
        d=65535;
     for(i=0;i<4;i++)
        if (w[i])
        {
        sd=&mapa.sidedef[sector1][i];
        sd->flags&=~0xFFF00f1f;
        if (d!=65535) sd->flags|=0x00100000;
        sd->flags|=flags1|(((flags1^flags2)<<24))|0x00400800;
        sd->prim=side1;
        sd->action=0;
        if (d!=65535)
           {
           sd->side_tag=d;
           sd->sector_tag=sector1;
           }
        d=i;
        }
     for(k=0;k<4;k++)
        if (w[k])
        {
        sd=&mapa.sidedef[w[k]][(k+2)&3];
        sd->flags&=~0xFFF00f1f;
        sd->flags|=flags1|(((flags1^flags2)<<24)|0x200);
        sd->prim=side1;
        sd->action=mode+5;
        sd->side_tag=d;
        sd->sector_tag=sector1;
        }
     }
  terminate_gui();
  }

void create_door(void)
  {
  int i;
  CTL3D b1,b2,b3;
  if (count_of_sel()!=1)
     {
     msg_box("Door Wizard:",'\x1',"Mus�� ozna�it pouze jeden sektor!","Ok",NULL);
     return;
     }
  for (i=0;i<maplen;i++) if (minfo[i].flags & 1) break;
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,50,400,300,"Door Wizard");
  define(CANCEL_BUTT,10,5,80,20,2,button,"Zru�it");property(&b1,NULL,NULL,WINCOLOR);
    on_control_change(terminate_gui);
  define(OK_BUTT,190,5,80,20,2,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);
    on_control_change(build_door);
  define(10,5,20,192,35,0,label,"Animace 1:   Po�et okenek:");property(&b3,NULL,NULL,WINCOLOR);
  define(20,10,35,90,12,0,str_line,side_names);property(&b2,NULL,NULL,WINCOLOR);c_default(0);
    on_enter(string_list_sup);
  define(30,120,35,50,12,0,input_line,10,1,16,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs(1));on_exit(test_int);
  define(40,5,60,192,35,0,label,"Animace 2:   Po�et okenek:");property(&b3,NULL,NULL,WINCOLOR);
  define(50,10,75,90,12,0,str_line,side_names);property(&b2,NULL,NULL,WINCOLOR);c_default(0);
    on_enter(string_list_sup);
  define(60,120,75,50,12,0,input_line,10,1,16,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs(1));on_exit(test_int);
  define(-1,220,20,100,10,0,label,"Vytvo�it dve�e na:");
  define(70,220,35,100,60,0,radio_butts,5,
                             "Severu",
                             "V�chodu",
                             "Jihu",
                             "Z�padu",
                             "Jako otev�rac� sektor");c_default(0);
  define(-1,5,100,390,160,0,label,"Detaily:");property(&b3,NULL,NULL,WINCOLOR);
  define(-1,200,110,0,140,0,label,""); property(&b2,NULL,NULL,WINCOLOR);
  define(-1,10,112,180,12,1,label,"Funkce dve��:");
  define(80,10,124,160,40,1,radio_butts,3,"Lze jenom otev��t",
                                          "Lze jenom zav��t",
                                          "Lze otev��t i zav��t");c_default(2);
  define(90,10,170,170,12,1,check_box,"D�lkov� ovl�d�n�");c_default(0);on_control_change(remote_acces);
  define(-1,10,112,190,12,0,label,"Kdy� jsou dve�e zav�en�");
  define(140,15,124,160,10,0,check_box,"Nepr�choz� hr��em");c_default(1);
  define(110,15,136,160,10,0,check_box,"Nepr�choz� nest�rou");c_default(1);
  define(120,15,148,160,10,0,check_box,"Nelze prohodit v�c");c_default(1);
  define(130,15,160,160,10,0,check_box,"Zvukot�sn�");c_default(1);
  define(100,15,172,160,10,0,check_box,"Nepr�hledn�");c_default(1);
  define(-1,10,190,190,12,0,label,"Kdy� jsou dve�e otev�en�");
  define(190,15,202,160,10,0,check_box,"Nepr�choz� hr��em");c_default(0);
  define(160,15,214,160,10,0,check_box,"Nepr�choz� nest�rou");c_default(0);
  define(170,15,226,160,10,0,check_box,"Nelze prohodit v�c");c_default(0);
  define(180,15,238,160,10,0,check_box,"Zvukot�sn�");c_default(0);
  define(150,15,250,160,10,0,check_box,"Nepr�hledn�");c_default(0);
  define(200,10,190,160,12,1,check_box,"Otev��t po startu");c_default(0);on_control_change(remote_acces);
  define(-1,10,210,160,12,1,label,"Lclip:");property(&b3,NULL,NULL,WINCOLOR);
  define(210,10,210,50,12,1,input_line,10,1,75,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs(1));on_exit(test_int);
  define(300,0,0,0,0,0,value_store,4);c_default(i);
  redraw_window();
  escape();
  close_window(waktual);
  }

void crt_oblouky()
{
  CTL3D b1,b2,b3;
  char v_rozich=1;

  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(635-140,50,140,100,"Oblouky");
  define(-1,5,20,70,10,0,label,"Zvol oblouk:");
  define(10,5,32,130,12,0,str_line,oblouky);property(&b2,NULL,NULL,WINCOLOR);c_default(0);
  on_enter(string_list_sup);
  define(20,5,45,100,10,0,check_box,"V rozich pouze");c_default(v_rozich);
  define(30,5,5,60,20,3,button,"Ok");on_control_change(terminate_gui);
  define(40,5,5,60,20,2,button,"Zru�it");on_control_change(terminate_gui);
  redraw_window();
  escape();
  if (o_aktual->id==30)
     {
     int obl,i,j;

     obl=f_get_value(0,10);
     v_rozich=f_get_value(0,20);
     for(i=1;i<maplen;i++)
        for(j=0;j<4;j++)
           if (minfo[i].flags & 1)
              if (mapa.sectordef[i].step_next[j] && !(mapa.sidedef[i][j].flags & 0x200))
              {
              mapa.sidedef[i][j].oblouk=obl;
              if (!obl) mapa.sidedef[i][j].flags&=~0x30000;
              else
                 if (v_rozich)
                 {
                 if (!(mapa.sidedef[i][(j+3)&3].flags & 0x200) &&
                     (mapa.sidedef[mapa.sectordef[i].step_next[j]][(j+3)&3].flags & 0x200 ||
                      mapa.sidedef[mapa.sectordef[i].step_next[(j+3)&3]][j].flags & 0x200))
                      mapa.sidedef[i][j].flags|=0x10000;
                 if (!(mapa.sidedef[i][(j+1)&3].flags & 0x200) &&
                     (mapa.sidedef[mapa.sectordef[i].step_next[j]][(j+1)&3].flags & 0x200 ||
                      mapa.sidedef[mapa.sectordef[i].step_next[(j+1)&3]][j].flags & 0x200))
                      mapa.sidedef[i][j].flags|=0x20000;
                 }
                 else
                 {
                 if (!(mapa.sidedef[i][(j+3)&3].flags & 0x80) ||
                    !(mapa.sidedef[mapa.sectordef[i].step_next[j]][(j+3)&3].flags & 0x80))
                    mapa.sidedef[i][j].flags|=0x10000;
                 if (!(mapa.sidedef[i][(j+1)&3].flags & 0x80) ||
                    !(mapa.sidedef[mapa.sectordef[i].step_next[j]][(j+1)&3].flags & 0x80))
                    mapa.sidedef[i][j].flags|=0x20000;
                 }
              }

     }
  close_window(waktual);
}

static void local_monsters_warning()
  {
  char *c="Lok�ln� nestv�ry";
  if (f_get_value(0,96))
     msg_box(c,'\x1',"Zapnut�m t�to volby zajist�te, �e seznam nestv�r se bude ukl�dat do souboru mapy, nikoliv do ENEMIES.DAT. Parametry nestv�r budou platit pouze pro tuto mapu","Ok",NULL);
  else
     msg_box(c,'\x1',"Vypnut�m t�to volby m��ete o n�kter� nestv�ry p�ij�t!","Ok",NULL);
  }

static void zabezpecit()
  {
  int i,j;
  char text[50],verify[50];

  j=msg_box("Zabezpe�en�",2,"Zabezbe�it lze mapu, nebo v�echny datov� soubory. Kter� heslo chce� zm�nit?","Mapu","Data","Zp�t",NULL);
  if (j==3) return;
  if (j==2) if (check_data_password()==0) return;
            else strcpy(text,set_data_password(NULL));
  if (j==1) strcpy(text,set_password(NULL));
  i=ask_password(text,1);
   if (i==1)
      {
      i=ask_password(verify,2);
      if (i==1)
       if (!strcmp(text,verify))
          {
          if (j==1) set_password(text);else set_data_password(text);
          }
       else msg_box("Nejsou schodn�",1,"Kontrola nesouhlasila. Heslo nebylo zm�n�no!","Ok",NULL);
      }
  }

void preference()
  {
  CTL3D b1,b2,b3;
  int i;

  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,50,350,290,"Globaln� volby mapy");
  define(-1,10,25,150,12,0,label,"Pozad� sever:");
  define(-1,10,45,150,12,0,label,"Pozad� v�chod:");
  define(-1,10,65,150,12,0,label,"Pozad� jih:");
  define(-1,10,85,150,12,0,label,"Pozad� zapad:");
  define(-1,10,105,150,12,0,label,"Barva �erven�:");
  define(-1,10,125,150,12,0,label,"Barva zelena:");
  define(-1,10,145,150,12,0,label,"Barva modra:");
  define(-1,10,165,150,12,0,label,"Start:");
  define(-1,10,185,150,12,0,label,"Start sm�r:");
  define(-1,10,25,150,12,3,label,"Jm�no urovn�:");
  for(i=0;i<4;i++)
     {
     define(10+10*i,150,25+20*i,100,15,0,input_line,12);set_default(mglob.back_fnames[i]);
     property(&b3,NULL,NULL,WINCOLOR);
     }
  define(50,150,105,50,15,0,input_line,5,0,255,"%5d");set_default(strs(mglob.fade_r));
     property(&b3,NULL,NULL,WINCOLOR);on_exit(test_int);
  define(60,150,125,50,15,0,input_line,5,0,255,"%5d");set_default(strs(mglob.fade_g));
     property(&b3,NULL,NULL,WINCOLOR);on_exit(test_int);
  define(70,150,145,50,15,0,input_line,5,0,255,"%5d");set_default(strs(mglob.fade_b));
     property(&b3,NULL,NULL,WINCOLOR);on_exit(test_int);
  define(80,150,165,50,15,0,input_line,5,1,maplen,"%5d");set_default(strs(mglob.start_sector));
     property(&b3,NULL,NULL,WINCOLOR);on_exit(test_int);
  define(90,150,185,100,60,0,radio_butts,4,
                             "Sever",
                             "V�chod",
                             "Jih",
                             "Z�pad");c_default(mglob.direction);
  define(95,210,105,100,60,0,radio_butts,5,
                             "Normaln�",
                             "Sopka (��r)",
                             "Ledov� j.(mr�z)",
                             "Pod vodou",
                             "M�sto");c_default(mglob.map_effector);
  define(96,210,165,100,10,0,check_box,"Lok�ln� nestv�ry");c_default(mglob.local_monsters);on_control_change(local_monsters_warning);
  define(97,210,180,100,10,0,check_box,"Autom.st�nuj p&s");c_default(mglob.map_autofadefc);
  define(CANCEL_BUTT,10,10,80,20,2,button,"Zru�it");on_control_change(terminate_gui);
  define(OK_BUTT,10,35,80,20,2,button,"Ok");on_control_change(terminate_gui);
  define(110,10,60,100,20,2,button,"Zabezpe�en�>>");on_control_change(zabezpecit);
  define(100,10,10,200,11,3,input_line,29);set_default(mglob.mapname);
  property(&b3,NULL,NULL,WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     char c;
     for(i=0;i<4;i++)
        get_value(0,i*10+10,mglob.back_fnames[i]);
     mglob.fade_r=vals(50);
     mglob.fade_g=vals(60);
     mglob.fade_b=vals(70);
     mglob.start_sector=vals(80);
     mglob.direction=f_get_value(0,90);
     mglob.map_effector=f_get_value(0,95);
     c=mglob.local_monsters;
     mglob.local_monsters=f_get_value(0,96);
     mglob.map_autofadefc=f_get_value(0,97);
     if (c!=mglob.local_monsters && c==1)
        {
        load_mobs();
        load_sound_map();
        }
     get_value(0,100,mglob.mapname);
     mglob.map_effector=f_get_value(0,95);
     }
  close_window(waktual);
  }

void set_side_flag(int sector,int side,int what,int set)
  {
  int value=1;

  value<<=what;
  if (set) mapa.sidedef[sector][side].flags|=value;
      else mapa.sidedef[sector][side].flags&=~value;
  }

void set_side_flags(int sector,int side,int on_flags,int off_flags,int depth)
  {
  if (on_flags & 1) set_side_flag(sector,side,7,1);
  if (on_flags & 2) set_side_flag(sector,side,18,1);
  if (on_flags & 4) set_side_flag(sector,side,29,1);
  if (on_flags & 8) set_side_flag(sector,side,9,1);
  if (on_flags & 0x10) set_side_flag(sector,side,8,1);
  if (off_flags & 1) set_side_flag(sector,side,7,0);
  if (off_flags & 2) set_side_flag(sector,side,18,0);
  if (off_flags & 4) set_side_flag(sector,side,29,0);
  if (off_flags & 8) set_side_flag(sector,side,9,0);
  if (off_flags & 0x10) set_side_flag(sector,side,8,0);
  if (on_flags & 0x20)
     {
     mapa.sidedef[sector][side].oblouk&=0x1f;
     mapa.sidedef[sector][side].oblouk|=depth<<5;
     }
  }

void set_flags_inside(int on_flags,int off_flags,int depth,int where)
  {
  int x1=+30000,y1=+30000,x2=-30000,y2=-30000;
  int i;

  for(i=0;i<maplen;i++)
    if (minfo[i].flags & 1)
     {
     if (minfo[i].x<x1) x1=minfo[i].x;
     if (minfo[i].y<y1) y1=minfo[i].y;
     if (minfo[i].x>x2) x2=minfo[i].x;
     if (minfo[i].y>y2) y2=minfo[i].y;
     }
  for(i=0;i<maplen;i++)
    if (minfo[i].flags & 1)
     {
     if (minfo[i].x==x1 && where & 8) set_side_flags(i,3,on_flags,off_flags,depth);
     if (minfo[i].x==x2 && where & 2) set_side_flags(i,1,on_flags,off_flags,depth);
     if (minfo[i].y==y1 && where & 1) set_side_flags(i,0,on_flags,off_flags,depth);
     if (minfo[i].y==y2 && where & 4) set_side_flags(i,2,on_flags,off_flags,depth);
     }
  }

void set_flags_outside(int on_flags,int off_flags,int depth,int where)
  {
  int x1=+30000,y1=+30000,x2=-30000,y2=-30000;
  int i;

  for(i=0;i<maplen;i++)
    if (minfo[i].flags & 1)
     {
     if (minfo[i].x<x1) x1=minfo[i].x;
     if (minfo[i].y<y1) y1=minfo[i].y;
     if (minfo[i].x>x2) x2=minfo[i].x;
     if (minfo[i].y>y2) y2=minfo[i].y;
     }
  for(i=0;i<maplen;i++)
    if (minfo[i].flags & 1)
     {
     int s;
     if (minfo[i].x==x1 && where & 8 && (s=mapa.sectordef[i].step_next[3])!=0) set_side_flags(s,1,on_flags,off_flags,depth);
     if (minfo[i].x==x2 && where & 2 && (s=mapa.sectordef[i].step_next[1])!=0) set_side_flags(s,3,on_flags,off_flags,depth);
     if (minfo[i].y==y1 && where & 1 && (s=mapa.sectordef[i].step_next[0])!=0) set_side_flags(s,2,on_flags,off_flags,depth);
     if (minfo[i].y==y2 && where & 4 && (s=mapa.sectordef[i].step_next[2])!=0) set_side_flags(s,0,on_flags,off_flags,depth);
     }
  }
void set_flags_everywhere(int on_flags,int off_flags,int depth,int where)
  {
  int i;

  for(i=0;i<maplen;i++)
    if (minfo[i].flags & 1)
     {
     if (where & 8) set_side_flags(i,3,on_flags,off_flags,depth);
     if (where & 2) set_side_flags(i,1,on_flags,off_flags,depth);
     if (where & 1) set_side_flags(i,0,on_flags,off_flags,depth);
     if (where & 4) set_side_flags(i,2,on_flags,off_flags,depth);
     }
  }

void open_blok_window()
  {
  int on_flags,off_flags,where,depth;

  def_dialoge(400,100,200,200,"Bloky");
  define(50,10,20,100,10,0,check_box,"Na Okraji");c_default(0);
  define(60,10,32,100,10,0,check_box,"Vn�");c_default(0);
  define(10,10,44,100,10,0,check_box,"Na severu");c_default(1);
  define(20,10,56,100,10,0,check_box,"Na v�chod�");c_default(1);
  define(30,10,68,100,10,0,check_box,"Na jihu");c_default(1);
  define(40,10,80,100,10,0,check_box,"Na z�pad�");c_default(1);
  define(-1,10,100,1,1,0,label,"Zap Vyp");
  define(100,40,112,150,10,0,check_box,"Pr�hlednost");c_default(0);
  define(110,40,124,150,10,0,check_box,"Dva druhy st�n");c_default(0);
  define(120,40,136,150,10,0,check_box,"Tajn� st�ny");c_default(0);
  define(130,40,148,150,10,0,check_box,"Prim. viditeln�");c_default(0);
  define(140,40,160,150,10,0,check_box,"Prim. animace");c_default(0);
  define(200,40,172,30,30,0,radio_butts,3,"-","\x4","\x6");property(NULL,icones,NULL,WINCOLOR);c_default(0);
  define(101,25,112,10,10,0,check_box,"");c_default(0);
  define(111,25,124,10,10,0,check_box,"");c_default(0);
  define(121,25,136,10,10,0,check_box,"");c_default(0);
  define(131,25,148,10,10,0,check_box,"");c_default(0);
  define(141,25,160,10,10,0,check_box,"");c_default(0);
  define(151,25,172,10,10,0,check_box,"");c_default(0);
  define(OK_BUTT,5,5,80,20,2,button,"Ok");on_control_change(terminate_gui);
  redraw_window();
  escape();
  on_flags=off_flags=depth=where=0;
  on_flags=get_bit_fields(0,101,6);
  off_flags=get_bit_fields(0,100,5);
  where=get_bit_fields(0,10,6);
  depth=f_get_value(0,200);
  if (where & 0x30)
     {
     if (where & 0x20) set_flags_outside(on_flags,off_flags,depth,where);
     if (where & 0x10) set_flags_inside(on_flags,off_flags,depth,where);
     }
  else set_flags_everywhere(on_flags,off_flags,depth,where);
  close_current();
  }

static char test_room(int x,int y,int layer)
  {
  int i;
  for(i=0;i<maplen;i++) if (minfo[i].x==x && minfo[i].y==y && minfo[i].layer==layer && mapa.sectordef[i].sector_type!=0) return 1;
  return 0;
  }

static void build_stairs()
  {
  int sect;
  char mode;
  char smer;
  char type;
  char *error=NULL;
  int target;
  int prim1,prim2,anim1,anim2;
  int dir1,dir2;
  int txtp1,txtp2;
  int tlayer;
  int x,y,clayer;
  static signed char smery[4][2]={{0,-1},{1,0},{0,1},{-1,0}};

  sect=f_get_value(0,300);
  mode=f_get_value(0,100);
  smer=f_get_value(0,70);dir1=smer;dir2=(smer+2)&3;
  type=f_get_value(0,80);
  target=vals(90);
  prim1=f_get_value(0,20);anim1=vals(30);
  prim2=f_get_value(0,50);anim2=vals(60);
  if (mode)
     if (type==2)
        if (!mapa.sectordef[sect].step_next[dir1])
           if (!mapa.sectordef[target].step_next[dir2])
              {
              mapa.sectordef[sect].step_next[dir1]=target;
              mapa.sectordef[target].step_next[dir2]=sect;
              mapa.sidedef[sect][dir1].flags&=~0x3;
              mapa.sidedef[target][dir2].flags&=~0x3;
              txtp1=sect;txtp2=target;
              }
           else error="Nelze vytvo�it schody na tento c�lovy sektor";
        else error="Nelze vytvo�it schody t�mto sm�rem. V�chod u� existuje";
     else error="P��m� spojen� lze tvo�it jen \"Na sektor\"";
  else if (type!=2)
     if ((txtp1=mapa.sectordef[sect].step_next[dir2]))
        {
        clayer=minfo[sect].layer;
        x=minfo[sect].x;
        y=minfo[sect].y;
        switch (type)
           {
           case 0:tlayer=clayer+1;break;
           case 1:tlayer=clayer-1;break;
           }
        if (!test_room(x,y,tlayer))
           {
           target=add_sector(x,y,tlayer,sect);
           if (target>0)
              {
              txtp2=target;
              if (!(txtp2=mapa.sectordef[target].step_next[dir1]))
                 if ((txtp2=add_sector(x+smery[dir1][0],y+smery[dir1][1],tlayer,0))<1) error="Nen� m�sto pro sektor!";
                 else wire_sector(txtp2);
              wire_sector(target);
              mapa.sectordef[sect].sector_tag=txtp2;
              mapa.sectordef[sect].sector_type=2;
              mapa.sectordef[target].sector_tag=txtp1;
              mapa.sectordef[target].sector_type=2;
              }
           else error="Nen� m�sto pro sektor!";
           }
        else error="V c�lov�m pat�e nen� m�sto pro sektor";
        }
     else error="Z toho sektoru nelze t�mto sm�rem vytvo�it schody";
  else
     if ((txtp1=mapa.sectordef[sect].step_next[dir2]))
        if ((txtp2=mapa.sectordef[target].step_next[dir1]))
           {
           mapa.sectordef[sect].sector_tag=txtp2;
           mapa.sectordef[sect].sector_type=2;
           mapa.sectordef[target].sector_tag=txtp1;
           mapa.sectordef[target].sector_type=2;
           }
        else error="Cilov� sektor m� chybn� orientovan� v�chody";
     else error="Nelze vytvo�it schody po�adovan�m sm�rem";
  if (error!=NULL)
     {
     msg_box("Stairs Wizard:",'\x1',error,"Ok",NULL);
     return;
     }
  if (!mode)
     {
     int i;
     for(i=0;i<4;i++)
        {
        mapa.sidedef[sect][i].flags=(i==dir2?SD_INVIS:0);
        mapa.sidedef[target][i].flags=(i==dir1?SD_INVIS:0);
        }
     mapa.sectordef[sect].floor=0;
     mapa.sectordef[sect].ceil=0;
     }
  mapa.sidedef[txtp1][dir1].prim=prim1;
  mapa.sidedef[txtp2][dir2].prim=prim2;
  mapa.sidedef[txtp1][dir1].prim_anim=anim1-1;
  mapa.sidedef[txtp2][dir2].prim_anim=anim2-1;
  mapa.sidedef[txtp1][dir1].flags|=0x280 | SD_INVIS;
  mapa.sidedef[txtp2][dir2].flags|=0x280 | SD_INVIS;
  mapa.sidedef[txtp1][dir1].flags&=~SD_DOUBLE_SIDE;
  mapa.sidedef[txtp2][dir2].flags&=~SD_DOUBLE_SIDE;
  if (anim1) mapa.sidedef[txtp1][dir1].flags|=0x100;
  if (anim2) mapa.sidedef[txtp2][dir2].flags|=0x100;
  terminate_gui();
  }

static void schody(void)
  {
  int i;
  CTL3D b1,b2,b3;
  if (count_of_sel()!=1)
     {
     msg_box("Stairs Wizard:",'\x1',"Mus�� ozna�it pouze jeden sektor!","Ok",NULL);
     return;
     }
  for (i=0;i<maplen;i++) if (minfo[i].flags & 1) break;
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,50,400,300,"Stairs Wizard");
  define(CANCEL_BUTT,10,5,80,20,2,button,"Zru�it");property(&b1,NULL,NULL,WINCOLOR);
    on_control_change(terminate_gui);
  define(OK_BUTT,190,5,80,20,2,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);
    on_control_change(build_stairs);
  define(10,5,20,192,35,0,label,"Tady:     Po�et okenek:");property(&b3,NULL,NULL,WINCOLOR);
  define(20,10,35,90,12,0,str_line,side_names);property(&b2,NULL,NULL,WINCOLOR);c_default(0);
    on_enter(string_list_sup);
  define(30,120,35,50,12,0,input_line,10,1,16,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs(1));on_exit(test_int);
  define(40,5,60,192,35,0,label,"Tam:      Po�et okenek:");property(&b3,NULL,NULL,WINCOLOR);
  define(50,10,75,90,12,0,str_line,side_names);property(&b2,NULL,NULL,WINCOLOR);c_default(0);
    on_enter(string_list_sup);
  define(60,120,75,50,12,0,input_line,10,1,16,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs(1));on_exit(test_int);
  define(-1,220,20,100,10,0,label,"Vytvo�it schody na:");
  define(70,220,35,100,60,0,radio_butts,4,
                             "Severu",
                             "V�chodu",
                             "Jihu",
                             "Z�padu");c_default(0);
  define(-1,5,100,390,160,0,label,"Detaily:");property(&b3,NULL,NULL,WINCOLOR);
  define(-1,200,110,0,140,0,label,""); property(&b2,NULL,NULL,WINCOLOR);
  define(-1,10,112,180,12,1,label,"Schody povedou:");
  define(80,10,124,160,40,1,radio_butts,3,"Nahoru",
                                          "Dolu",
                                          "Na sektor");c_default(2);
  define(-1,10,112,180,12,0,label,"��slo sektoru:");
  define(90,130,112,50,12,0,input_line,10,1,maplen,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs(1));on_exit(test_int);
  define(100,10,130,180,10,0,check_box,"P��m� spojen�");c_default(0);
  define(300,0,0,0,0,0,value_store,4);c_default(i);
  redraw_window();
  escape();
  close_window(waktual);

  }

void predvolba(void)
  {
     unselect_map();
     edit_side(0,0);
     sector_details_call(0);
  }

static void vyber(void)
  {
  CTL3D b1,b2,b3;
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,50,300,200,"Vyber podle kriterii");
  define(CANCEL_BUTT,10,5,80,20,2,button,"Zru�it");property(&b1,NULL,NULL,WINCOLOR);
    on_control_change(terminate_gui);
  define(OK_BUTT,100,5,80,20,2,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);
    on_control_change(terminate_gui);
  define(-1,10,20,60,10,0,label,"Vyber st�nu kterou chce� ozna�it:");
  define(400,10,40,100,10,0,check_box,"Severn�");c_default(0);
  define(410,10,55,100,10,0,check_box,"V�chodn�");c_default(0);
  define(420,10,70,100,10,0,check_box,"Ji�n�");c_default(0);
  define(430,10,85,100,10,0,check_box,"Z�padn�");c_default(0);
  define(440,10,100,200,10,0,check_box,"Podle prim�rn� st�ny");c_default(0);
  define(20,10,115,200,12,0,str_line,side_names);property(&b2,NULL,NULL,WINCOLOR);c_default(0);
    on_enter(string_list_sup);
  define(450,10,130,200,10,0,check_box,"Podle sekund�rn� st�ny");c_default(0);
  define(30,10,145,200,12,0,str_line,side_names);property(&b2,NULL,NULL,WINCOLOR);c_default(0);
    on_enter(string_list_sup);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
  {
	long smer=get_bit_fields(0,400,4);
	int prim_side=f_get_value(0,20);
	int sec_side=f_get_value(0,30);
	long pro_prim=get_bit_fields(0,440,1);
	long pro_sec=get_bit_fields(0,450,1);
	int i;
	for(i=1;i<maplen;i++) if (minfo[i].flags & 1)
	{
	  int j;
	  for (j=0;j<4;j++) if ((1<<j) & smer)
	  {
		if (mapa.sectordef[i].step_next[j]==0 || (pro_prim && mapa.sidedef[i][j].flags & SD_PRIM_VIS) || (pro_sec && mapa.sidedef[i][j].flags & SD_SEC_VIS))
		{
		  char select=(!pro_prim && !pro_sec) || (pro_prim && mapa.sidedef[i][j].prim==prim_side) ||
			(pro_sec && mapa.sidedef[i][j].sec==sec_side);
		  if (select) continue;
		}
		minfo[i].flags&=~1;
	  }
	}
  }
  close_window(waktual);

  /*
  int s;
  int i=msg_box("V�b�r",2,"Zvol jeden ze �ty� filtr� vyb�ru nebo akci zru� klepnut�m na Zru�it",
     "Sever","V�chod","Jih","Z�pad","Zru�it",NULL);
  if (i==5) return;
  i--;
  for(s=1;s<maplen;s++)
     if (minfo[s].flags & 1 && mapa.sectordef[s].step_next[i]!=0) minfo[s].flags &= ~1;
	 */
  }

void open_wiz_tool(void)
  {
  FC_TABLE f_sel;
  WINDOW *w;

  w=waktual;
  if (find_window(wiz_tool_numb)==NULL)
  {
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  memcpy(&f_sel,flat_color(RGB555(0,0,23)),sizeof(charcolors));
  wiz_tool_numb=def_window(120,200,"Map Wizard");
  waktual->y=2;waktual->x=SCR_WIDTH_X-120-3;
  on_control_change(close_wiz_tool);
  define(10,10,25,100,19,0,button2,"Dve�e");on_control_change(create_door);
  define(10,10,45,100,19,0,button2,"Oblouky");on_control_change(crt_oblouky);
  define(10,10,65,100,19,0,button2,"Schody");on_control_change(schody);
  define(10,10,85,100,19,0,button2,"Globaln�");on_control_change(preference);
  define(10,10,105,100,19,0,button2,"Bloky");on_control_change(open_blok_window);
  define(10,10,125,100,19,0,button2,"Multiakce");on_control_change(open_multiaction_window);
  define(10,10,145,100,19,0,button2,"P�edvolba");on_control_change(predvolba);
  define(10,10,165,100,19,0,button2,"V�b�r");on_control_change(vyber);
  }
  else
  {
  select_window(wiz_tool_numb);
  }
  redraw_window();
  select_window(w->id);
  }

static BOOL WINAPI SetCurrentAppWindow(HWND hWnd, LPARAM lParam)
{
  HWND *hPtr=(HWND *)lParam;
  *hPtr=hWnd;
  return FALSE;
}

typedef struct __BrowserDlgDesc
{  
  char *path;
  char *filename;  
  int baselen;
  int level;
} BrowserDlgDesc;

#include "../mapedit/resource.h"
#include <windowsx.h>

static void DirListbox(const char *mask, HWND listBox, int dots)
{
  WIN32_FIND_DATA w32fnd;
  HANDLE h;

  if (strrchr(mask,'\\') == 0)
  {
      char *dirmask=(char *)alloca(strlen(mask)+5);
      strcpy(dirmask,".\\");
      strcat(dirmask,mask);
      DirListbox(dirmask,listBox,dots);
      return;
  }
  h=FindFirstFile(mask,&w32fnd);
  ComboBox_ResetContent(listBox);
  if (h!=INVALID_HANDLE_VALUE) do
  {
    char *name=w32fnd.cFileName;
    if (~w32fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      int p=ComboBox_AddString(listBox,name);
      ComboBox_SetItemData(listBox,p,0);
    }
  } while (FindNextFile(h,&w32fnd));
  FindClose(h);
  {
  char *dirmask=(char *)alloca(strlen(mask)+5);
  char *c=strrchr(mask,'\\');
  HANDLE h;
  if (c==0) c=dirmask;else c++;
  strcpy(dirmask,mask);
  strcpy(dirmask+(c-mask),"*.*");  
  h=FindFirstFile(dirmask,&w32fnd);
  if (h!=INVALID_HANDLE_VALUE) do
  {
    char *name=w32fnd.cFileName;
    if (w32fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if (strcmp(name,"."))
      {
        if (dots||strcmp(name,".."))
        {
          int p;
          name=(char *)alloca(strlen(name)+3);
          sprintf(name,"[%s]",w32fnd.cFileName);
          p=ComboBox_AddString(listBox,name);
          ComboBox_SetItemData(listBox,p,1);
        }    
      }
    }
  } while (FindNextFile(h,&w32fnd));
  FindClose(h);
  }
  }


LRESULT WINAPI BrowserDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  BrowserDlgDesc *dlgnfo=(BrowserDlgDesc *)GetWindowLong(hDlg,DWL_USER);
  switch (msg)
  {
  case WM_INITDIALOG:
      dlgnfo=(BrowserDlgDesc *)lParam;
      DirListbox(dlgnfo->path,GetDlgItem(hDlg,IDC_LIST),0);
      SetWindowLong(hDlg,DWL_USER,lParam);            
      dlgnfo->baselen=strrchr(dlgnfo->path,'\\')-dlgnfo->path+1;
      dlgnfo->level=0;
      {
        HWND parent=GetParent(hDlg);
        RECT rc1,rc2;
        GetWindowRect(parent,&rc1);
        GetWindowRect(hDlg,&rc2);
        rc2.left=((rc1.right+rc1.left)>>1)-((rc2.right-rc2.left)>>1);
        rc2.top=((rc1.top+rc1.bottom)>>1)-((rc2.bottom-rc2.top)>>1);
        SetWindowPos(hDlg,0,rc2.left,rc2.top,0,0,SWP_NOZORDER|SWP_NOSIZE);        
        SetDlgItemText(hDlg,IDC_PATH,dlgnfo->path);
      }
      break;
  case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
      case IDOK:
        {
          HWND list=GetDlgItem(hDlg,IDC_LIST);
          int len=GetWindowTextLength(list)+1;
          char *name=(char *)alloca(len+10);
          int sel;
          int mode=0;
          char *c;

          GetWindowText(list,name,len);
          if (name[0]==0) break;
          sel=ComboBox_FindStringExact(list,-1,name);
          if (sel!=-1) mode=ComboBox_GetItemData(list,sel);
          if (mode==0)
          {
            if (strlen(name)<5 || stricmp(name+strlen(name)-4,".map")!=0) strcat(name,".map");
            if (strlen(name)>12) 
            {
              MessageBox(hDlg,"Jmeno mapy je prilis dlouhe (max 8 znaku bez pripony nebo 12 znaku s priponou)",0,MB_OK|MB_ICONEXCLAMATION);
              break;
            }
            strcpy(dlgnfo->filename,dlgnfo->path+dlgnfo->baselen);
            c=strrchr(dlgnfo->filename,'\\');          
            if (strlen(name)+strlen(dlgnfo->filename)>=MAX_PATH) break;
            if (c==0) c=dlgnfo->filename;else c++;
            strcpy(c,name);
            EndDialog(hDlg, IDOK);
          }
          else
          {
            strcpy(name,name+1);
            name[strlen(name)-1]=0;
            if (strcmp(name,"..")==0)
            {
              char *a=strrchr(dlgnfo->path,'\\');
              char *b;
              if (a==0) break;
              *a=0;
              b=strrchr(dlgnfo->path,'\\');
              if (b==0) b=dlgnfo->path;else b++;
              strcpy(b,a+1);
              dlgnfo->level--;
            }
            else
            {
              char *c=(char *)malloc(strlen(dlgnfo->path)+strlen(name)+5);
              char *a;
              char *b;
              strcpy(c,dlgnfo->path);
              a=strrchr(c,'\\');
              if (a == 0) a = c; else a++;
              b=a+strlen(name)+1;
              memmove(b,a,strlen(a)+1);
              a[strlen(name)] = '\\';
              memcpy(a,name,strlen(name));
              dlgnfo->level++;
              free(dlgnfo->path);
              dlgnfo->path=c;
            }
            DirListbox(dlgnfo->path,list,dlgnfo->level);
            SetDlgItemText(hDlg,IDC_PATH,dlgnfo->path);
          }
        }
        break;
        case IDCANCEL:
          EndDialog(hDlg,IDCANCEL);
          break;
        case IDC_LIST: if (HIWORD(wParam)==LBN_DBLCLK)
                         PostMessage(hDlg,WM_COMMAND,IDOK,0);
          break;
        default: return 0;
      }
    }
    break;
  default: return 0;
  }
  return 1;
}

void browser(const char *path, char *filename)
{
  BrowserDlgDesc dlg;
  HWND hWnd;
  dlg.path=strdup(path);
  dlg.filename=filename;
  EnumThreadWindows(GetCurrentThreadId(),SetCurrentAppWindow,(LPARAM)&hWnd);
  if (DialogBoxParam(GetModuleHandle(0),MAKEINTRESOURCE(IDD_OPENDIALOG),hWnd,BrowserDlgProc,(LPARAM)&dlg)==IDCANCEL)
    filename[0]=0;
  free(dlg.path);
}
/*
void browser(char *pathname,char *filename)
  {
  OPENFILENAME ofn;
  char *mask=(char *)alloca(strlen(pathname)*2+10);
  char *c;
  char tempfile[MAX_PATH];
  char *cwd=getcwd(NULL,0);
  char *basePath=strcat(strcat(strcpy((char *)alloca(strlen(cwd)+strlen(pathname)+3),cwd),"\\"),pathname);
  char *z=strrchr(basePath,'\\');
  z[0]=0;
  
  c=strrchr(pathname,'\\');
  if (c!=0) c++;else c=pathname;

  ZeroMemory(&ofn,sizeof(ofn));
  ofn.lStructSize=sizeof(ofn);
  sprintf(mask,"%s%c%s%c",c,0,c,0);

  EnumThreadWindows(GetCurrentThreadId(),SetCurrentAppWindow,(LPARAM)&(ofn.hwndOwner));
  ofn.Flags=OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
  ofn.lpstrDefExt=strrchr(pathname,'.')+1;
  ofn.lpstrFile=tempfile;
  ofn.lpstrFilter=mask;
  ofn.nMaxFile=MAX_PATH;
  ofn.lpstrInitialDir=basePath;
opakuj:
  tempfile[0]=0;
  if (GetOpenFileName(&ofn))
  {

	if (strnicmp(basePath,tempfile,strlen(basePath))!=0)
	{
	   msg_box("Browser",'\x1',"Nelze vybirat soubory nad aktualni slozkou.","Ok",NULL);
	   goto opakuj;
	}
	strcpy(filename,tempfile+strlen(basePath)+1);	
  }
  else
	filename[0]=0;
  free(cwd);
}
  /*
  TSTR_LIST l;

  filename;
  l=read_directory(pathname,DIR_BREIF,_A_NORMAL);
  if (l==NULL && default_ext)
     {
     msg_box("Browser",'\x1',"Funkce read_directory() vratila chybu. Adres�� je pr�zdn� nebo je nedostatek pam�ti","Ok",NULL);
     return;
     }
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_window(160,450,"Nalistovat");
  set_window_modal();
  on_change(terminate);
  define(9,10,20,120,420,0,listbox,l,RGB555(31,31,0),0);c_default(0);
  on_change(terminate);
  o_end->autoresizex=1;
  o_end->autoresizey=1;
  define(10,3,42,21,360,1,scroll_bar_v,0,10,1,SCROLLBARCOL);
  property(NULL,NULL,NULL,WINCOLOR);
  o_end->autoresizey=1;
  define(11,3,20,21,17,1,scroll_button,-1,0,"\x1e");
  property(NULL,icones,NULL,WINCOLOR);on_change(scroll_support);
  define(12,3,22,21,17,2,scroll_button,1,10,"\x1f");
  property(NULL,icones,NULL,WINCOLOR);on_change(scroll_support);
  define(20,3,1,10,10,2,resizer);
  redraw_window();
  escape();
  if (o_aktual->id==9)
     {
     char *a,*b;
     strcpy(filename,l[f_get_value(0,9)]);
     a=filename;b=a;
     while (*b || b-filename>12)
       {
       *a=*b++;
       if (*a!=32) a++;
       }
     *a='\0';
     }

  close_window(waktual);
  release_list(l);
  */

void add_ma_flags(int start,int x,int y,TMULTI_ACTION *p)
  {
  define(-1,x,y,1,1,0,label,"Podm�nky");
  define(start+0,x+25,y+15,140,10,0,check_box,"Cancel program");c_default(p->general.cancel);
  define(start+1,x+25,y+27,140,10,0,check_box,"Jedenkr�t");c_default(p->general.once);
  define(start+2,x+25,y+39,140,10,0,check_box,"�sp��n� pr�chod");c_default(p->general.ps);
  define(start+3,x+25,y+51,140,10,0,check_box,"Ne�sp��n� pr�chod");c_default(p->general.pf);
  define(start+4,x+25,y+63,140,10,0,check_box,"�sp��n� dotyk");c_default(p->general.ts);
  define(start+5,x+25,y+75,140,10,0,check_box,"Ne�sp��n� dotyk");c_default(p->general.tf);
  define(start+6,x+25,y+87,140,10,0,check_box,"Informace o z�mku");c_default(p->general.li);
  define(start+7,x+25,y+99,140,10,0,check_box,"P�ed odchodem");c_default(p->general.ul);
  define(start+8,x+25,y+111,140,10,0,check_box,"P�i p��chodu akce");c_default(p->general.ia);
  define(start+9,x+25,y+123,140,10,0,check_box,"P�i startu levelu");c_default(p->general.sp);
  define(start+10,x+25,y+135,140,10,0,check_box,"P�i uzav�en� dve��");c_default(p->general.as);
  define(start+11,x+25,y+147,140,10,0,check_box,"Ka�de anim.pol��ko");c_default(p->general.am);
  define(start+12,x+25,y+159,140,10,0,check_box,"Ka�de druh� a.p.");c_default(p->general.a2);
  define(start+13,x+25,y+171,140,10,0,check_box,"P�i proveden� akce");c_default(p->general.pa);
  define(start+14,x+25,y+183,140,10,0,check_box,"�sp��n� SpecProc");c_default(p->general.us);
  define(start+15,x+25,y+195,140,10,0,check_box,"P�i otev�en� dve��");c_default(p->general.od);
  define(start+16,x+25,y+207,140,10,0,check_box,"Ud�lost v�klenku");c_default(p->general.vy);
  }

void read_ma_flags(int start,TMULTI_ACTION *p)
  {
  p->general.cancel=f_get_value(0,start+0);
  p->general.once=f_get_value(0,start+1);
  p->general.ps=f_get_value(0,start+2);
  p->general.pf=f_get_value(0,start+3);
  p->general.ts=f_get_value(0,start+4);
  p->general.tf=f_get_value(0,start+5);
  p->general.li=f_get_value(0,start+6);
  p->general.ul=f_get_value(0,start+7);
  p->general.ia=f_get_value(0,start+8);
  p->general.sp=f_get_value(0,start+9);
  p->general.as=f_get_value(0,start+10);
  p->general.am=f_get_value(0,start+11);
  p->general.a2=f_get_value(0,start+12);
  p->general.pa=f_get_value(0,start+13);
  p->general.us=f_get_value(0,start+14);
  p->general.od=f_get_value(0,start+15);
  p->general.vy=f_get_value(0,start+16);
  }


void *load_wav_sample(char *filename,long *size)
  {
  FILE *f;
  char *c;
  void *d;

  concat(c,sample_path,filename);
  f=fopen(c,"rb");
  if (f==NULL)
    {
    return afile(filename,read_group(0),size);
    }
  fseek(f,0,SEEK_END);
  d=malloc(*size=ftell(f));
  fseek(f,0,SEEK_SET);
  fread(d,1,*size,f);
  fclose(f);
  return d;
  }


void tma_sound_update()
  {
  int select;
  TSTR_LIST ls;
  char *riff,*rff,*rff2;
  char fmt_buff[256];
  T_WAVE fmt;
  long size;
  char *c;

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  select=f_get_value(0,9);
  concat(c,"",ls[select]);
  if (c==NULL) return;
  name_conv(c);
  riff=load_wav_sample(c,&size);
  if (riff==NULL) return;
  if ((rff=find_chunk(riff,WAV_FMT))!=NULL && (rff2=find_chunk(riff,WAV_DATA))!=NULL)
     {
     read_chunk(rff,&fmt_buff);
     size=get_chunk_size(rff2);
     memcpy(&fmt,fmt_buff,sizeof(fmt));
     if (fmt.wav_mode==1 && fmt.chans==1)
        {
        strcpy(c,ls[select]);
        name_conv(c);
        set_value(0,20,c);
        sprintf(c,"%6d",fmt.freq);set_value(0,30,c);
        sprintf(c,"%6d",size);set_value(0,40,c);
        sprintf(c,"%6d",size);set_value(0,50,c);
        sprintf(c,"%6d",0);set_value(0,60,c);
        c_set_value(0,100,fmt.freq!=fmt.bps);
        }
     else msg_box("Nespr�vn� WAV",'\x1',"Do hry lze vlo�it jen WAV ve formatu PCM mono!","Ok",NULL);
     }
  else msg_box("Chybn� soubor WAV",'\x1',"Tento soubor neobsahuje platn� format pro WAV","Ok",NULL);
  free(riff);
  }



void play_wav(va_list args)
  {
  char *filename=va_arg(args,char *);
  int ffreq=va_arg(args,int);
  char fmt_buff[256];
  T_WAVE fmt;
  char *riff,*rff;
  static load_another=2;
  static char *sample=NULL;
  int freq,mode,samplesize;
  int startloop,endloop,ofs;
  long size;
  char argsSave[16];
  memcpy(argsSave,args,16);

  riff=load_wav_sample(filename,&size);
  if (riff==NULL) return;
  if ((rff=find_chunk(riff,WAV_FMT))==NULL) return;
     {
     read_chunk(rff,&fmt_buff);
     memcpy(&fmt,fmt_buff,sizeof(fmt));
     if (fmt.wav_mode==1 && fmt.chans==1)
        {
        freq=fmt.freq;
        mode=fmt.bps/freq;
        if (load_another==2) start_mixing();
        load_another=1;
        task_sleep(NULL);
        task_sleep(NULL);
        load_another=0;
        if ((rff=find_chunk(riff,WAV_DATA))!=NULL)
        {
        samplesize=get_chunk_size(rff);
        if (sample!=NULL) free(sample);
        sample=(char *)getmem(samplesize);
        read_chunk(rff,sample);
           {
           int *z;
           set_channel_volume(0,32767,32767);
/*           if (mode==1)
              for(i=0;i<samplesize && !load_another;sample[i]^=0x80,i++) ;*/
           //else if (mode==2) for(i=1;i<samplesize && !load_another;sample[i]^=0x80,i+=2);
           z=(int *)argsSave;
           if (ffreq>0)
              {
              freq=ffreq;
              startloop=z[0];
              endloop=z[1];
              ofs=z[2];
              if (endloop>samplesize)
                 {
                 msg_box("Pozor",'\x1',"Konec opakov�n� le�� za koncem souboru!","Ok",NULL);
                 endloop=samplesize;
                 }
              if (startloop>endloop)
                 {
                 msg_box("Pozor",'\x1',"Za��tek opakov�n� le�� ZA jeho koncem!","Ok",NULL);
                 startloop=endloop;
                 }
              }
           else
              {
              startloop=samplesize;
              endloop=samplesize;
              ofs=0;
              }
           play_sample(0,sample+ofs,endloop-ofs,startloop-ofs,freq,mode);
           while (!load_another && get_channel_state(0)) task_sleep(NULL);
           mute_channel(0);
           }
        free(sample);sample=NULL;
        }
        }
     }
  free(riff);
  if (load_another==0)
     {
     stop_mixing();
     load_another=2;
     }
  }

void tma_sound_preview()
  {
  char ok;
  char au;
  int select;
  TSTR_LIST ls;
  char *c;

  ok=f_get_value(0,290);
  au=f_get_value(0,295);
  if (au) tma_sound_update();
  if (!ok) return;
  send_message(E_GUI,9,E_CONTROL,0,&ls);
  select=f_get_value(0,9);
  if (ls[select]==NULL) return;
  concat(c,"",ls[select]);
  name_conv(c);
  add_task(16384,play_wav,c,-1);
  do_events();
  do_events();
  }

void tma_sound_autoupdate()
  {
  char c;

  c=f_get_value(0,295);
  set_enable(0,320,!c);
  }

void tma_sound_test()
  {
  char *c;
  char d[15];

  get_value(0,20,d);
  concat(c,"",d);
  name_conv(c);
  add_task(16384,play_wav,c,vals(30),vals(40),vals(50),vals(60));
  do_events();
  do_events();
  }

void read_ddl_list_wav(char ***list)
  {
  TSTR_LIST ddld;
  int i,c;

  ddld=read_ddl_dir("WAV");
  if (ddld==NULL) return;
  if (*list==NULL)
    {
    *list=ddld;
    return;
    }
  c=str_count(ddld);
  str_add(list,"--- SKELDAL ---");
  for(i=0;i<c;i++) if (ddld[i]!=NULL) str_add(list,ddld[i]);
  release_list(ddld);
  }

void tma_sound_stop()
  {
  mute_channel(0);
  }

void tma_sound(TMULTI_ACTION *q)
  {
  CTL3D b1,b2,b3;
  TSTR_LIST list;
  static char preview=1;
  static char autoupdate=1;
  char *c;

  if (q->sound.freq==0)
     {
     q->sound.freq=11025;
     q->sound.volume=100;
     }
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,50,350,390,"Editor zvukov�ch efekt�");
  concat(c,sample_path,"*.wav");
  list=read_directory(c,DIR_BREIF,_A_NORMAL);
  read_ddl_list_wav(&list);
  define(9,10,20,200,126,0,listbox,list,RGB555(31,31,31),0);
  property(&b3,NULL,NULL,WINCOLOR);c_default(0);on_control_change(tma_sound_preview);
  define(10,217,40,19,87,0,scroll_bar_v,0,10,1,SCROLLBARCOL);
  property(&b2,NULL,NULL,WINCOLOR);
  define(11,216,20,21,17,0,scroll_button,-1,0,"\x1e");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(12,216,130,21,17,0,scroll_button,1,10,"\x1f");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(-1,10,160,100,11,0,label,"Jm�no:");
  define(-1,10,175,100,11,0,label,"freq[Hz]:");
  define(-1,10,190,100,11,0,label,"StartLoop:");
  define(-1,10,205,100,11,0,label,"EndLoop:");
  define(-1,10,220,100,11,0,label,"SmpOffset:");
  define(-1,10,235,100,11,0,label,"Hlasitost:");
  define(-1,10,250,100,11,0,label,"Stopa:");
  define(20,90,160,110,11,0,input_line,12);
  property(&b3,NULL,NULL,WINCOLOR);set_default(q->sound.filename);
  define(30,90,175,70,11,0,input_line,7,4000,88000,"%6d");on_exit(test_int);
  property(&b2,NULL,NULL,WINCOLOR);set_default(strs(q->sound.freq));
  define(40,90,190,70,11,0,input_line,7,0,0x7fffffff,"%6d");on_exit(test_int);
  property(&b2,NULL,NULL,WINCOLOR);set_default(strs(q->sound.start_loop));
  define(50,90,205,70,11,0,input_line,7,0,0x7fffffff,"%6d");on_exit(test_int);
  property(&b2,NULL,NULL,WINCOLOR);set_default(strs(q->sound.end_loop));
  define(60,90,220,70,11,0,input_line,7,0,0x7fffffff,"%6d");on_exit(test_int);
  property(&b2,NULL,NULL,WINCOLOR);set_default(strs(q->sound.offset));
  define(70,90,235,70,11,0,input_line,7,0,100,"%6d");on_exit(test_int);
  property(&b2,NULL,NULL,WINCOLOR);set_default(strs(q->sound.volume));
  define(80,90,250,70,11,0,input_line,7,0,100,"%6d");on_exit(test_int);
  property(&b2,NULL,NULL,WINCOLOR);set_default(strs(q->sound.soundid));
  define(100,20,265,100,11,0,check_box,"16Bit");c_default(q->sound.bit16 & 1);
  define(110,20,277,100,11,0,check_box,"Zaka� p�i otev�r�n�");c_default((q->sound.bit16 & 2)>>1);
  define(120,20,290,100,11,0,check_box,"Zaka� p�i zav�r�n�");c_default((q->sound.bit16 & 4)>>2);
  define(130,20,302,100,11,0,check_box,"N�hodn� stereo");c_default((q->sound.bit16 & 8)>>3);
  define(290,5,80,100,11,1,check_box,"Preview");c_default(preview);
  define(295,5,92,100,11,1,check_box,"AutoUpdate");c_default(autoupdate);on_control_change(tma_sound_autoupdate);
  define(300,5,20,100,20,1,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);
   on_control_change(terminate_gui);
  define(310,5,45,100,20,1,button,"Zrusit");property(&b1,NULL,NULL,WINCOLOR);
   on_control_change(terminate_gui);
  define(320,5,160,100,20,1,button,"<< Update");property(&b1,NULL,NULL,WINCOLOR);
   on_control_change(tma_sound_update);
  define(330,5,135,45,20,1,button,"");property(&b1,NULL,NULL,WINCOLOR);
   on_control_change(tma_sound_test);
  define(340,60,135,45,20,1,button,"");property(&b1,NULL,NULL,WINCOLOR);
   on_control_change(tma_sound_stop);
  add_ma_flags(400,165,175,q);
  redraw_desktop();
  set_enable(0,320,!autoupdate);
  escape();
  preview=f_get_value(0,290);
  autoupdate=f_get_value(0,295);
  if (o_aktual->id==300)
     {
     char c[15];
     read_ma_flags(400,q);
     q->sound.volume=vals(70);
     q->sound.start_loop=vals(40);
     q->sound.end_loop=vals(50);
     q->sound.freq=vals(30);
     q->sound.offset=vals(60);
     q->sound.soundid=vals(80);
     q->sound.bit16=get_bit_fields(0,100,4);
     get_value(0,20,c);
     strncpy(q->sound.filename,c,12);
     }
  close_current();
  mute_channel(0);
  }

//----------------------- WIZARD MULTIACTIONS -------------------------

TMULTI_ACTION *add_multiaction(int sector,int side,int before, TMULTI_ACTION *typa)
  {
  TMULTI_ACTION **p;
  int type;

  type=typa->general.action;
  p=multi_actions[sector*4+side];
  if (p==NULL)
     {
     p=(TMULTI_ACTION **)getmem(sizeof(TMULTI_ACTION *));
     memset(p,0,_msize(p));
     *p=(TMULTI_ACTION *)getmem(action_sizes[type]);
     memset(*p,0,action_sizes[type]);
     multi_actions[sector*4+side]=p;
     memcpy(*p,typa,action_sizes[type]);
     return *p;
     }
  else
   {
   TSTR_LIST z;
   char *zs;
   TMULTI_ACTION *c;

   z=(TSTR_LIST)p;
   zs=(char *)getmem(action_sizes[type]);
   memset(zs,1,action_sizes[type]);
   zs[action_sizes[type]-1]=0;
   c=(TMULTI_ACTION *)str_insline(&z,before,zs);
   multi_actions[sector*4+side]=(TMULTI_ACTION **)z;
   memset(c,0,action_sizes[type]);
   memcpy(c,typa,action_sizes[type]);
   free(zs);
   return c;
   }
  }

void delete_multiaction(int sector,int side,int pos)
  {
  TSTR_LIST p;

  p=(TSTR_LIST)multi_actions[sector*4+side];
  if (p==NULL) return;
  str_remove(&p,pos);
  str_delfreelines(&p);
  if (*p==NULL)
     {
     release_list(p);
     p=NULL;
     }
  multi_actions[sector*4+side]=(TMULTI_ACTION **)p;

  }

void prefer_multiaction(int sector,int side,int pos)
  {
  TSTR_LIST p;
  int i;
  char *c;

  p=(TSTR_LIST)multi_actions[sector*4+side];
  if (p==NULL) return;
  i=pos;
  while (i>1)
     {
     c=p[i];
     p[i]=p[i-1];
     p[i+1]=c;
     i--;
     }
  }

void create_multiaction_list(TSTR_LIST *list,int sector,int side)
  {
  TMULTI_ACTION **p;
  int count,i,j;
  int list_count;
  char s[200];

  p=multi_actions[sector*4+side];
  if (*list==NULL) *list=create_list(10);
  list_count=str_count(*list);
  for(i=0;i<list_count;i++) str_remove(list,i);
  if (p) count=str_count((TSTR_LIST) p); else count=0;
  j=0;
  for(i=0;i<count;i++)
     if (p[i]!=NULL)
        {
        sprintf(s,"%2d%c%c %s (0%X)",j++,p[i]->general.cancel?'X':'_',p[i]->general.once?'1':'_',act_types[p[i]->general.action],p[i]->sound.flags+p[i]->sound.eflags*256);
        str_add(list,s);
        }
  sprintf(s,"<end>");
  str_add(list,s);
  str_delfreelines(list);
  }

void init_multiactions()
  {
  static char again=0;
  int i,s,j;
  TMULTI_ACTION **p;

  if (again) for(i=0;i<MAPSIZE*4;i++)
     if ((p=multi_actions[i])!=NULL)
        {

        s=_msize(p)/4;
        for(j=0;j<s;j++) if (p[j]!=NULL) free(p[j]);
        free(p);
        }
  memset(multi_actions,0,sizeof(multi_actions));
  again=1;
  }

void show_3dbox_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  int xs=x2-x1,ys=y2-y1;
  int ysp2=ys*2/10;
  int xsp2=xs/4,ysp1=ys*4/10;

  o;
  curcolor=0;
  hor_line(x1,y2,x2);
  hor_line(x1,y1,x2);
  ver_line(x1,y1,y2);
  ver_line(x2,y1,y2);
  curcolor=RGB555(8,8,8);
  line(x1,y1,x1+xsp2,y1+ysp2);line(x2,y1,x2-xsp2,y1+ysp2);
  line(x1,y2,x1+xsp2,y2-ysp1);line(x2,y2,x2-xsp2,y2-ysp1);
  hor_line(x1+xsp2,y2-ysp1,x2-xsp2);
  hor_line(x1+xsp2,y1+ysp2,x2-xsp2);
  ver_line(x1+xsp2,y1+ysp2,y2-ysp1);
  ver_line(x2-xsp2,y1+ysp2,y2-ysp1);
  set_aligned_position((x1+x2)/2,y2,1,2,"Y");outtext("Y");
  set_aligned_position(x2,(y1+y2)/2,2,1,"Z");outtext("Z");
  set_aligned_position(x1+xsp2/2,y2-ysp1/2,0,0,"X");outtext("X");

  }

void show_3dbox(OBJREC *o)
  {
  o->call_draw=show_3dbox_draw;
  }

void tma_fireball(TMULTI_ACTION *p)
  {
  CTL3D *b1;
  static int item=1,xpos=-32,ypos=250,zpos=160,speed=16;
  char new;
  TSTR_LIST ls_sorts=NULL;

  create_isort_list(&ls_sorts,-1);
  def_dialoge(220,60,400,300,"Macro: Fireball");
  b1=def_border(5,WINCOLOR);
  new=0;
  if (p->fireball.item==0)
     {
     p->fireball.item=item;
     p->fireball.xpos=xpos;
     p->fireball.ypos=ypos;
     p->fireball.zpos=zpos;
     p->fireball.speed=speed;
     new=1;
     }
  define(-1,5,20,1,1,0,label,"Vyst�elen� v�c:");
  define(-1,5,35,1,1,0,label,"Xpoz <-63,63>:");
  define(-1,5,50,1,1,0,label,"Ypoz <0,499>:");
  define(-1,5,65,1,1,0,label,"Zpoz <0,319>:");
  define(-1,5,80,1,1,0,label,"Rychlost:<1,128>");
  define(-1,5,100,120,120,1,show_3dbox);
  add_ma_flags(100,5,100,p);
  define(10,150,20,200,12,0,str_line,ls_sorts);property(b1,NULL,NULL,WINCOLOR);
  c_default (p->fireball.item-1);on_enter(string_list_sup);
  define(20,150,35,60,12,0,input_line,3,-63,63,"%3d");property(b1,NULL,NULL,WINCOLOR);
  set_default(strs(p->fireball.xpos));on_exit(test_int);
  define(30,150,50,60,12,0,input_line,3,0,499,"%3d");property(b1,NULL,NULL,WINCOLOR);
  set_default(strs(p->fireball.ypos));on_exit(test_int);
  define(40,150,65,60,12,0,input_line,3,0,319,"%3d");property(b1,NULL,NULL,WINCOLOR);
  set_default(strs(p->fireball.zpos));on_exit(test_int);
  define(50,150,80,60,12,0,input_line,3,0,319,"%3d");property(b1,NULL,NULL,WINCOLOR);
  set_default(strs(p->fireball.speed));on_exit(test_int);
  b1=def_border(1,0);
  define(60,10,10,80,20,2,button,"Zru�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(70,10,35,80,20,2,button,"Ok");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_window();
  escape();
  if (o_aktual->id==70)
     {
     p->fireball.xpos=vals(20);
     p->fireball.ypos=vals(30);
     p->fireball.zpos=vals(40);
     p->fireball.speed=vals(50);
     p->fireball.item=f_get_value(0,10)+1;
     if (new)
        {
        xpos=vals(20);
        ypos=vals(30);
        zpos=vals(40);
        speed=vals(50);
        item=f_get_value(0,10)+1;
        }
     read_ma_flags(100,p);
     }
  close_current();
  release_list(ls_sorts);
  }

void tma_send(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Send Action");
  define(-1,5,22,1,1,0,label,"Sektor:");
  b1=def_border(5,WINCOLOR);
  define(10,80,20,60,12,0,input_line,5,0,maplen,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->send_a.sector));
  define(-1,5,37,1,1,0,label,"St�na:");
  define(20,80,35,100,12,0,str_line,steny2);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->send_a.side);
  define(-1,5,52,1,1,0,label,"Akce:");
  define(30,80,50,150,12,0,str_line,actions);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->send_a.s_action);
  define(-1,5,67,1,1,0,label,"Zpo�d�n�:");
  define(40,80,65,60,12,0,input_line,5,0,255,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->send_a.delay));
  define(110,10,90,200,10,0,check_box,"Zm�na automapingu");
  define(120,10,102,200,10,0,check_box,"Zm�na pr�chodnosti(hr��)");
  define(130,10,114,200,10,0,check_box,"Zm�na pr�chodnosti(nestv�ra)");
  define(140,10,126,200,10,0,check_box,"Zm�na pr�chodnosti(v�c)");
  define(150,10,138,200,10,0,check_box,"Zm�na pr�chodnosti(zvuk)");
  add_ma_flags(200,220,20,q);
  fill_bit_fields(0,110,q->send_a.change_bits,5);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->send_a.sector=vals(10);
     q->send_a.side=f_get_value(0,20);
     q->send_a.s_action=f_get_value(0,30);
     q->send_a.delay=vals(40);
     q->send_a.change_bits=get_bit_fields(0,110,5);
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_loadlev(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Load Level");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Sektor:");
  define(20,80,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->loadlev.start_pos));
  define(-1,5,37,1,1,0,label,"Sm�r:");
  define(30,80,35,100,12,0,str_line,steny2);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->loadlev.dir);
  define(-1,5,52,1,1,0,label,"Jm�no:");
  define(10,80,50,100,12,0,input_line,12);property(b1,NULL,NULL,WINCOLOR);set_default(q->loadlev.name);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->loadlev.start_pos=vals(20);
     q->loadlev.dir=f_get_value(0,30);
     get_value(0,10,q->loadlev.name);
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_play_anim(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Play animation");
  b1=def_border(5,WINCOLOR);
  define(-1,5,20,1,1,0,label,"Jm�no:");
  define(10,80,22,100,12,0,input_line,12);property(b1,NULL,NULL,WINCOLOR);set_default(q->loadlev.name);
  define(20,20,40,100,10,0,check_box,"cls");c_default(q->loadlev.dir);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->loadlev.dir=f_get_value(0,20);
     get_value(0,10,q->loadlev.name);
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_write_book(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  if (q->loadlev.name[0]==0) strcpy(q->loadlev.name,"KNIHA.TXT");
  def_dialoge(220,60,410,300,"Macro: Write to book");
  b1=def_border(5,WINCOLOR);
  define(-1,5,20,1,1,0,label,"Kniha:");
  define(10,80,35,100,12,0,input_line,12);property(b1,NULL,NULL,WINCOLOR);set_default(q->loadlev.name);
  define(-1,5,50,1,1,0,label,"��slo odstavce:");
  define(20,80,65,65,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->loadlev.start_pos));
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->loadlev.start_pos=vals(20);
     get_value(0,10,q->loadlev.name);
     read_ma_flags(200,q);
     }
  close_current();
  }


void tma_swapsectors(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Swap Sectors");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Sektor1:");
  define(20,80,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->swaps.sector1));
  define(-1,5,37,1,1,0,label,"Sektor2:");
  define(30,80,35,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->swaps.sector2));
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->swaps.sector1=vals(20);
     q->swaps.sector2=vals(30);
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_experience(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Send Experience");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Zku�enost:");
  define(20,100,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm1));
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->twop.parm1=vals(20);
     read_ma_flags(200,q);
     }
  close_current();
  }



void tma_cancel_action(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Cancel Action");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Sektor:");
  define(20,80,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->cactn.sector));
  define(-1,5,37,1,1,0,label,"Sm�r:");
  define(30,80,35,100,12,0,str_line,steny2);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->cactn.dir);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->cactn.sector=vals(20);
     q->cactn.dir=f_get_value(0,30);
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_sharema(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Share Multiaction");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Sektor:");
  define(20,80,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm1));
  define(-1,5,37,1,1,0,label,"Sm�r:");
  define(30,80,35,100,12,0,str_line,steny2);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->twop.parm2);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  define(-1,5,100,10,10,0,label,"Spust� MA program na jin�m");
  define(-1,5,110,10,10,0,label,"m�st�, tak jako by byl");
  define(-1,5,120,10,10,0,label,"definov�n zde.");
  define(-1,5,140,10,10,0,label,"Vyu�it�:");
  define(-1,5,150,10,10,0,label,"Sd�len� jednoho program");
  define(-1,5,160,10,10,0,label,"Zm�na v m�st� sd�leni se pak");
  define(-1,5,170,10,10,0,label,"projev� na v�ech m�stech, kter�");
  define(-1,5,180,10,10,0,label,"program sd�lej�.");
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->twop.parm1=vals(20);
     q->twop.parm2=f_get_value(0,30);
     read_ma_flags(200,q);
     }
  close_current();
  }


void tma_teleport_group(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Teleport Group");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Sektor:");
  define(20,80,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm1));
  define(-1,5,37,1,1,0,label,"Sm�r:");
  define(30,80,35,100,12,0,str_line,steny2);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->twop.parm2 & 3);
  define(40,80,59,100,10,0,check_box,"Povol efekt");c_default(q->twop.parm2>>7);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->twop.parm1=vals(20);
     q->twop.parm2=f_get_value(0,30)+(f_get_value(0,40)<<7);
     read_ma_flags(200,q);
     }
  close_current();
  }


void tma_lock(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Lock");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"��slo kl��e:");
  define(10,120,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->lock.key_id));
  define(-1,5,37,1,1,0,label,"Urove�:");
  define(20,120,35,60,12,0,input_line,5,-1,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->lock.thieflevel));
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->lock.key_id=vals(10);
     q->lock.thieflevel=vals(20);
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_specproc(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Call Specproc");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Jm�no spec:");
  define(10,120,20,95,12,0,str_line,wall_procs);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->lock.key_id);
  define(-1,5,37,1,1,0,label,"Parametr:");
  define(20,120,35,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->lock.thieflevel));
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->twop.parm1=f_get_value(0,10);
     q->twop.parm2=vals(20);
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_ifjump(TMULTI_ACTION *q,int linenum,TSTR_LIST ls)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: If Jump");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Podm�nka:");
  define(10,10,35,200,12,0,str_line,ls);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(abs(q->twop.parm1)-1);
  define(-1,5,52,1,1,0,label,"��slo ��dku:");
  define(20,120,50,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm2+linenum));
  define(30,5,75,100,30,0,radio_butts,2,"Skok kdy� podm�nka plat�","Skok kdy� podm�nka neplat�");
  c_default(q->twop.parm1<0);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->twop.parm1=f_get_value(0,10)+1;
     q->twop.parm2=vals(20)-linenum;
     if (f_get_value(0,30)) q->twop.parm1=-q->twop.parm1;
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_randjmp(TMULTI_ACTION *q,int linenum)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: If Jump");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Pravd�podobnost: [%]");
  define(10,120,35,60,12,0,input_line,5,0,100,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm1));
  define(-1,5,52,1,1,0,label,"��slo ��dku:");
  define(20,120,50,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm2+linenum));
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->twop.parm1=vals(10);
     q->twop.parm2=vals(20)-linenum;
     read_ma_flags(200,q);
     }
  close_current();
  }


void tma_ifflagjump(TMULTI_ACTION *q,int linenum)
  {
  CTL3D *b1=NULL;

  if (q->twop.parm1==0) q->twop.parm1=1;
  def_dialoge(220,60,410,300,"Macro: If flag Jump");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"��slo vlajky:");
  define(10,120,20,60,12,0,input_line,5,0,255,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(abs(q->twop.parm1)-1));
  define(-1,5,52,1,1,0,label,"��slo ��dku:");
  define(20,120,50,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm2+linenum));
  define(30,5,75,100,30,0,radio_butts,2,"Skok kdy� vlajka nastavena","Skok kdy� vlajka vynulov�na");
  c_default(q->twop.parm1<0);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->twop.parm1=vals(10)+1;
     q->twop.parm2=vals(20)-linenum;
     if (f_get_value(0,30)) q->twop.parm1=-q->twop.parm1;
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_setflag(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Change Flag");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"��slo vlajky:");
  define(10,120,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm1));
  define(-1,5,37,1,1,0,label,"Akce:");
  define(20,60,37,80,40,0,radio_butts,3,"Nulovat","Nastavit","Negovat");c_default(q->twop.parm2);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->twop.parm1=vals(10);
     q->twop.parm2=f_get_value(0,20);
     read_ma_flags(200,q);
     }
  close_current();
  }


void tma_wound(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: HIT Player");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Min:");
  define(10,120,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->wound.minor));
  define(-1,5,37,1,1,0,label,"Max:");
  define(20,120,35,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->wound.major));
  define(30,120,50,100,10,0,check_box,"Ohroz� skupinu");c_default(q->wound.pflags & 1);
  define(40,120,70,60,80,0,radio_butts,7,"P��m� zran�n�","Hod na obranu","Ohe�","Voda","Zem�","Vzduch","Mysl");
  c_default(q->wound.pflags>>1);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->wound.minor=vals(10);
     q->wound.major=vals(20);
     q->wound.pflags=f_get_value(0,30)+(f_get_value(0,40)<<1);
     read_ma_flags(200,q);
     }
  close_current();
  }

void tma_cmoney(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Create Money");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Min:");
  define(10,120,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm1));
  define(-1,5,37,1,1,0,label,"Max:");
  define(20,120,35,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm2));
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  tady:
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     int a=vals(10),b=vals(20);
     if (a>b)
        { msg_box("Create Money",1,"Min mus� b�t men�� ne� Max","OK",NULL);goto tady;}
     q->twop.parm1=a;
     q->twop.parm2=b;
     read_ma_flags(200,q);
     }
  close_current();
  }

static void tma_gomonster(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: Send Monster To");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Odkud:");
  define(10,120,20,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm1));
  define(-1,5,37,1,1,0,label,"Kam:");
  define(20,120,35,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->twop.parm2));
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  tady:
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     int a=vals(10),b=vals(20);
     if (a==b)
        { msg_box("Send Monster To",1,"Ale tohle nesmysl!","OK",NULL);goto tady;}
     q->twop.parm1=a;
     q->twop.parm2=b;
     read_ma_flags(200,q);
     }
  close_current();
  }


void tma_clock(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;
  char s[20];

  def_dialoge(220,60,410,300,"Macro: Code Lock");
  b1=def_border(5,WINCOLOR);
  define(-1,5,22,1,1,0,label,"Character:");
  define(10,100,20,10,12,0,input_line,2);
  property(b1,NULL,NULL,WINCOLOR);s[0]=q->clock.znak;s[1]=0;set_default(s);
  define(-1,5,37,1,1,0,label,"String:");
  define(20,80,35,100,12,0,input_line,8);strncpy(s,q->clock.string,8);s[8]=0;set_default(s);
  property(b1,NULL,NULL,WINCOLOR);
  define(-1,5,52,1,1,0,label,"��slo:");
  define(30,80,50,30,12,0,input_line,8,0,15," %02d");set_default(strs(q->clock.codenum));
  on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     get_value(0,10,s);q->clock.znak=s[0];
     get_value(0,20,s);strncpy(q->clock.string,s,8);
     q->clock.codenum=vals(30);
     read_ma_flags(200,q);
     }
  close_current();
  }




void tma_text(TMULTI_ACTION *q,char *filename)
  {
  CTL3D *b1;
  int id;
  TSTR_LIST ls;
  int lcount;


  ls=create_list(10);
  if (id=load_string_list(&ls,filename))
     {
     char s[300];
     if (id<0) sprintf(s,"Soubour %s chyb� nebo je po�kozen. Nelze editovat texty",filename);
     else sprintf(s,"Chyba v souboru %s na ��dce %d",filename,id);
     msg_box("Chyba!",'\x1',s,"Ok",NULL);
     return;
     }
  lcount=str_count(ls);
  if (q->text.textindex>=lcount) q->text.textindex=0;
  while (q->text.textindex<lcount && ls[q->text.textindex]==NULL)
       (q->text.textindex)++;
  if (q->text.textindex>=lcount)
     {
     msg_box("Ozn�men�",'\x1',"��dn� texty nejsou k dispozici","Ok",NULL);
     return;
     }
  b1=def_border(5,WINCOLOR);
  def_dialoge(420,120,250,290,"Oprav akci");
  define(-1,5,20,1,1,0,label,"Text:");
  define(10,5,19,170,12,1,str_line,ls);
    c_default(q->text.textindex);
    on_enter(string_list_sup);
    property(b1,NULL,flat_color(0x1f),WINCOLOR);
  add_ma_flags(20,5,35,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,5,60,20,3,button,"Ok");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(CANCEL_BUTT,5,5,60,20,2,button,"Zru�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_window();
  escape();
  id=o_aktual->id;
  if (id==CANCEL_BUTT)
     {
     release_list(ls);
     close_current();
     return;
     }
  read_ma_flags(20,q);
  q->text.textindex=f_get_value(0,10);
  release_list(ls);
  close_current();
  }

void tma_dialog(TMULTI_ACTION *q)
  {
  CTL3D *b1;
  int id;
  int i;


  i=pgf2name(q->text.textindex);
  b1=def_border(5,WINCOLOR);
  def_dialoge(420,120,250,290,"Oprav akci");
  define(-1,5,20,1,1,0,label,"Dialog:");
  define(10,5,19,170,12,1,str_line,dlg_names);
    c_default(i);
    on_enter(string_list_sup);
    property(b1,NULL,flat_color(0x1f),WINCOLOR);
  add_ma_flags(20,5,35,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,5,60,20,3,button,"Ok");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(CANCEL_BUTT,5,5,60,20,2,button,"Zru�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_window();
  escape();
  id=o_aktual->id;
  if (id==CANCEL_BUTT)
     {
     close_current();
     return;
     }
  read_ma_flags(20,q);
  q->text.textindex=dlg_pgfs[f_get_value(0,10)];
  close_current();
  }

void tma_shop(TMULTI_ACTION *q)
  {
  CTL3D *b1;
  int id;
  int i;
  TSTR_LIST ls=NULL;


  re_build_shop_list(&ls,shop_list,max_shops);
  for(i=0;i<max_shops;i++) if (shop_list[i].shop_id==q->text.textindex) break;
  b1=def_border(5,WINCOLOR);
  def_dialoge(420,120,250,290,"Oprav akci");
  define(-1,5,20,1,1,0,label,"Obchod:");
  define(10,5,19,170,12,1,str_line,ls);
    c_default(i);
    on_enter(string_list_sup);
    property(b1,NULL,flat_color(0x1f),WINCOLOR);
  add_ma_flags(20,5,35,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,5,60,20,3,button,"Ok");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(CANCEL_BUTT,5,5,60,20,2,button,"Zru�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_window();
  escape();
  release_list(ls);
  id=o_aktual->id;
  if (id==CANCEL_BUTT)
     {
     close_current();
     return;
     }
  read_ma_flags(20,q);
  q->text.textindex=shop_list[f_get_value(0,10)].shop_id;
  close_current();
  }


void tma_create_dropi(TMULTI_ACTION *q,char *name)
  {
  CTL3D *b1;
  int id;
  TSTR_LIST ls=NULL;
  int lcount;
  int it;


  create_isort_list(&ls,-1);
  lcount=str_count(ls);
  it=q->dropi.item;
  if (it>=lcount) it=0;
  b1=def_border(5,WINCOLOR);
  def_dialoge(420,120,250,290,name);
  define(-1,5,20,1,1,0,label,"V�c:");
  define(10,5,19,170,12,1,str_line,ls);
    c_default(it);
    on_enter(string_list_sup);
    property(b1,NULL,flat_color(0x1f),WINCOLOR);
  add_ma_flags(20,5,35,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,5,60,20,3,button,"Ok");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(CANCEL_BUTT,5,5,60,20,2,button,"Zru�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_window();
  escape();
  id=o_aktual->id;
  release_list(ls);
  if (id==CANCEL_BUTT)
     {
     close_current();
     return;
     }
  read_ma_flags(20,q);
  q->dropi.item=f_get_value(0,10);
  close_current();
  }


void tma_gen(TMULTI_ACTION *q,char *name)
  {
  CTL3D *b1;
  int id;


  b1=def_border(5,WINCOLOR);
  def_dialoge(420,120,250,290,name);
  add_ma_flags(20,5,35,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,5,60,20,3,button,"Ok");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(CANCEL_BUTT,5,5,60,20,2,button,"Zru�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_window();
  escape();
  id=o_aktual->id;
  if (id==CANCEL_BUTT)
     {
     close_current();
     return;
     }
  read_ma_flags(20,q);
  close_current();
  }


static void tma_unique_edit()
  {
  TITEM *it;

  it=(TITEM *)f_get_value(0,20);
  item_edit(it);
  }

void tma_unique(TMULTI_ACTION *q,char edit)
  {
  CTL3D *b1;


  if (!edit)
     {
     int i;
     tma_create_dropi(q,"Zvol p�edlohu");
     i=q->dropi.item;
     memcpy(&q->uniq.item,item_list+i,sizeof(q->uniq.item));
     }
  def_dialoge(420,120,250,290,"Create Unique");
  b1=def_border(1,0);
  define(10,10,19,230,15,1,button,"Oprav vlastnosti predmetu");
    on_control_change(tma_unique_edit);
    property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(20,0,0,0,0,0,value_store,4);c_default((int)&q->uniq.item);
  add_ma_flags(30,5,40,q);
  define(OK_BUTT,5,5,60,20,3,button,"Zav��t");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_window();
  if (edit)escape();else tma_unique_edit();
  read_ma_flags(30,q);
  close_current();
  }

static char *global_events[MAGLOB_NEXTID]=
{
  "Pred opustenim mapy",
  "Pred usnutim",
  "Po probuzeni",
  "Pred ulozenim",
  "Po ulozeni",
  "Pred carovanim",
  "Po carovani",
  "Otevreni mapy",
  "Uzavreni mapy",
  "Pred bitvou",
  "Po bitve",
  "Otevreni knihy",
  "Zavreni knihy",
  "Kazde kolo",
  "Smrt postavy muze",
  "Smrt postavy zeny",
  "Smrt vsech postav",
  "Zraneni muze",
  "Zraneni zeny",
  "Sebrani runy",
  "Sebrani predmetu",
  "Krok",
  "Otoceni",
  "Alarm",
  "Magie ohne",
  "Magie vody",
  "Magie zeme",
  "Magie vzduchu",
  "Magie mysli",
  "Pri kouzle 1 <param=id>",
  "Pri kouzle 2 <param=id>",
  "Pri kouzle 3 <param=id>",
  "Pri kouzle 4 <param=id>",
  "Pri kouzle 5 <param=id>",
  "Pri kouzle 6 <param=id>",
  "Pri kouzle 7 <param=id>",
  "Pri kouzle 8 <param=id>",
  "Pri kouzle 9 <param=id>",
  "Casovac 1 <kola/-cas>",
  "Casovac 2 <kola/-cas>",
  "Casovac 3 <kola/-cas>",
  "Casovac 4 <kola/-cas>",
  "Melodie fletny 1 <id melodie>",
  "Melodie fletny 2 <id melodie>",
  "Melodie fletny 3 <id melodie>",
  "Melodie fletny 4 <id melodie>",
  "Melodie fletny 5 <id melodie>",
  "Melodie fletny 6 <id melodie>",
  "Melodie fletny 7 <id melodie>",
  "Melodie fletny 8 <id melodie>",
};

void tma_globe(TMULTI_ACTION *q)
  {
  CTL3D *b1=NULL;

  int i,cnt=sizeof(global_events)/sizeof(global_events[0]);
  TSTR_LIST lst=create_list(cnt);
  for (i=0;i<cnt;i++) str_add(&lst,global_events[i]);

  def_dialoge(220,60,410,300,"Macro: Global Event");

  define(-1,5,22,1,1,0,label,"Udalost:");
  b1=def_border(5,WINCOLOR);

  define(5,80,20,200,12,0,str_line,lst);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->globe.event);
  define(6,290,20,60,12,0,input_line,10,-0x7FFFFFFF,0x7FFFFFFF,"%d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->globe.param));
  define(-1,5,52,1,1,0,label,"Sektor:");
  define(10,80,50,60,12,0,input_line,10,0,maplen,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->globe.sector));
  define(-1,5,37,1,1,0,label,"Pri vzniku udalosti poslat akci na:");
  define(-1,5,67,1,1,0,label,"St�na:");
  define(20,80,65,100,12,0,str_line,steny2);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->globe.side);
  define(-1,5,80,100,10,0,label,"P�i volbe sektor 0 severni");
  define(-1,5,90,100,10,0,label,"se akce neposila");
  define(30,20,110,100,10,0,check_box,"Zakaz puvodni chovani");c_default(q->globe.cancel);
  add_ma_flags(200,200,50,q);
  fill_bit_fields(0,110,q->send_a.change_bits,5);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->globe.event=f_get_value(0,5);
     q->globe.param=vals(6);
     q->globe.sector=vals(10);
     q->globe.side=f_get_value(0,20);
     q->globe.cancel=get_bit_fields(0,30,1);
     read_ma_flags(200,q);
     }
  close_current();
  release_list(lst);
  }

static char create_mode=0;

void tma_sectnumjmp(TMULTI_ACTION *q,int linenum)
  {
  CTL3D *b1=NULL;

  def_dialoge(220,60,410,300,"Macro: If Jump");
  b1=def_border(5,WINCOLOR);
  define(-1,5,52,1,1,0,label,"Sektor:");
  define(10,80,50,60,12,0,input_line,6,0,maplen,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->ifsec.sector));
  define(-1,5,67,1,1,0,label,"St�na:");
  define(20,80,65,100,12,0,str_line,steny2);on_enter(string_list_sup);
  property(b1,NULL,NULL,WINCOLOR);c_default(q->ifsec.side);
  define(-1,5,82,1,1,0,label,"��slo ��dku:");
  define(30,120,80,60,12,0,input_line,5,0,32767,"%5d");on_exit(test_int);
  property(b1,NULL,NULL,WINCOLOR);set_default(strs(q->ifsec.line+linenum));
  define(40,5,120,120,30,0,radio_butts,2,"Skok kdy� podm�nka plat�","Skok kdy� podm�nka neplat�");
  c_default(q->ifsec.invert);
  add_ma_flags(200,220,20,q);
  b1=def_border(1,0);
  define(OK_BUTT,5,30,80,20,2,button,"Ok");on_control_change(terminate_gui);property(b1,NULL,flat_color(0x1f),WINCOLOR);
  define(CANCEL_BUTT,5,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui);property(b1,NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     q->ifsec.sector=vals(10);
	 q->ifsec.side=f_get_value(0,20);
     q->ifsec.line=vals(30)-linenum;
     q->ifsec.invert=f_get_value(0,40);
     read_ma_flags(200,q);
     }
  close_current();
  }

void edit_multiaction()
  {
  int sect,side,num;
  TMULTI_ACTION *q;
  TMULTI_ACTION **z;
  TSTR_LIST ls=NULL;

  sect=vals(10);
  side=f_get_value(0,20);
  num=f_get_value(0,29);
  z=multi_actions[sect*4+side];if (z==NULL) return;
  if (num>=str_count((TSTR_LIST)(z))) return;
  q=z[num];if (q==NULL) return;
  switch (q->general.action)
     {
     case MA_GEN:return;
     case MA_SOUND:tma_sound(q);break;
     case MA_TEXTG:tma_text(q,TEXT_FILE);break;
     case MA_TEXTL:tma_text(q,pripona(filename,TXT));break;
     case MA_SENDA:tma_send(q);break;
     case MA_FIREB:tma_fireball(q);break;
     case MA_LOADL:tma_loadlev(q);break;
     case MA_DROPI:tma_create_dropi(q,"Macro: Drop item");break;
     case MA_DIALG:tma_dialog(q);break;
     case MA_SSHOP:tma_shop(q);break;
     case MA_CLOCK:tma_clock(q);break;
     case MA_CACTN:tma_cancel_action(q);break;
     case MA_LOCK:tma_lock(q);break;
     case MA_SWAPS:tma_swapsectors(q);break;
     case MA_WOUND:tma_wound(q);break;
     case MA_IFJMP:tma_ifjump(q,num,side_flgs);break;
     case MA_CALLS:tma_specproc(q);break;
     case MA_PICKI:
     case MA_HAVIT:create_isort_list(&ls,-1);tma_ifjump(q,num,ls);release_list(ls);break;
     case MA_STORY:tma_text(q,pripona(filename,TXT));break;
     case MA_IFACT:tma_ifjump(q,num,actions);break;
     case MA_SNDEX:tma_experience(q);break;
     case MA_TELEP:tma_teleport_group(q);break;
     case MA_PLAYA:tma_play_anim(q);break;
     case MA_CREAT:tma_create_dropi(q,"Macro: Create item");break;
     case MA_ISFLG:tma_ifflagjump(q,num);break;
     case MA_CHFLG:tma_setflag(q);break;
     case MA_GUNIQ:
     case MA_CUNIQ:tma_unique(q,q->uniq.item.jmeno[0]!=0);break;
     case MA_MONEY:tma_cmoney(q);break;
     case MA_WBOOK:tma_write_book(q);break;
     case MA_RANDJ:tma_randjmp(q,num);break;
     case MA_GOMOB:tma_gomonster(q);break;
     case MA_SHRMA:if (create_mode) q->sound.flags=q->sound.eflags=0xFF;tma_sharema(q);break;
     case MA_MUSIC:tma_text(q,pripona(filename,TXT));break;
	 case MA_GLOBE:tma_globe(q);break;
	 case MA_IFSEC:tma_sectnumjmp(q,num);break;
	 case MA_IFSTP:tma_ifjump(q,num,sector_types);break;
     default:if (!create_mode) tma_gen(q,act_types[q->general.action]);break;
     }
  create_multiaction_list(&mul_list,sect,side);
  send_message(E_GUI,29,E_CONTROL,1,mul_list);
  create_mode=0;
  }
void create_multiaction_invalid()
  {
  int i;

  i=f_get_value(0,10);
  set_enable(0,OK_BUTT,i<(sizeof(action_sizes)/sizeof(int)));
  }



void create_multiaction_dialoge()
  {
  CTL3D *b1;
  int id,i;
  static TMULTI_ACTION x={0,0,0,0};
  int sect,side,before;
  char s[30];
  static int act_default=1;
  TSTR_LIST ls;char *c;

  sect=vals(10);
  ls=(TSTR_LIST)getmem(_msize(act_types));
  memcpy(ls,act_types,_msize(act_types));
  sort_list(ls,-1);
  c=act_types[act_default];id=str_count(act_types);
  for(i=0;i<id;i++) if (c==ls[i]) break;
  side=f_get_value(0,20);
  before=f_get_value(0,29);
  sprintf(s,"Vytvo� akci %d",before);
  def_dialoge(250,200,350,290,s);
  b1=def_border(5,WINCOLOR);
  define(-1,5,20,1,1,0,label,"P��kaz:");
  define(9,10,40,120,206,0,listbox,ls,RGB555(31,31,31),0);on_control_change(create_multiaction_invalid);
  property(b1,NULL,NULL,WINCOLOR);c_default(-1);
  define(10,136,60,21,167,0,scroll_bar_v,0,10,1,SCROLLBARCOL);
  property(b1,NULL,NULL,WINCOLOR);
  define(11,136,40,21,17,0,scroll_button,-1,0,"\x1e");
  property(b1,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(12,136,230,21,17,0,scroll_button,1,10,"\x1f");
  property(b1,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  //define(10,5,19,130,12,1,str_line,act_types);c_default(act_default);on_enter(string_list_sup);
  add_ma_flags(20,136,20,&x);
  b1=def_border(1,0);
  define(OK_BUTT,5,5,60,20,3,button,"Vyvo�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(CANCEL_BUTT,5,5,60,20,2,button,"Zru�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  c_set_value(0,9,i);
  send_message(E_GUI,9,E_CONTROL,2);
  redraw_window();
  escape();
  id=o_aktual->id;
  if (id==CANCEL_BUTT)
     {
     close_current();
     return;
     }
  read_ma_flags(20,&x);
  c=ls[f_get_value(0,9)];id=str_count(act_types);
  for(i=0;i<id;i++) if (c==act_types[i]) break;
  if (i==id) {close_current();return;}
  act_default=x.general.action=i;
  free(ls);
  add_multiaction(sect,side,before,&x);
  close_current();
  create_mode=1;
  edit_multiaction();
  c_set_value(0,29,before+1);
  create_multiaction_list(&mul_list,sect,side);
  send_message(E_GUI,29,E_CONTROL,1,mul_list);
  }

void delete_multiaction_ask()
  {
  int sect,side,num;

  sect=vals(10);
  side=f_get_value(0,20);
  num=f_get_value(0,29);
  if (msg_box("Potvrdit",'\x2',"Chce� tento p��kaz skute�n� zru�it?","Ano","Ne",NULL)==2) return;
  delete_multiaction(sect,side,num);
  if (num>0) c_set_value(0,29,num-1);
  create_multiaction_list(&mul_list,sect,side);
  send_message(E_GUI,29,E_CONTROL,1,mul_list);
  }

void mult_change_dir()
  {
  int i,s;


  i=vals(10);
  s=f_get_value(0,20);
  create_multiaction_list(&mul_list,i,s);
  send_message(E_GUI,29,E_CONTROL,1,mul_list);
  }

static TMULTI_ACTION **pamet=0;

static void copy_and_paste()
{
  CTL3D *b1=NULL;
  int id;

  int sect=vals(10);
  int side=f_get_value(0,20);

  def_dialoge(400,100,200,320,"Pr�ce s pam�t�");
  b1=def_border(5,WINCOLOR);
  define(10,10,20,180,20,0,button,"Kop�rovat do pam�ti");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(400,10,70,100,10,0,check_box,"Severn�");c_default(side==0);
  define(410,10,85,100,10,0,check_box,"V�chodn�");c_default(side==1);
  define(420,10,100,100,10,0,check_box,"Ji�n�");c_default(side==2);
  define(430,10,115,100,10,0,check_box,"Z�padn�");c_default(side==3);
  define(440,10,130,180,10,0,check_box,"Tam kde je prim. st�na");c_default(0);
  define(445,10,150,180,12,0,str_line,side_names);property(b1,NULL,NULL,WINCOLOR);c_default(0);
    on_enter(string_list_sup);
  define(450,10,170,180,10,0,check_box,"Tam kde je sec. st�na");c_default(0);
  define(455,10,190,180,12,0,str_line,side_names);property(b1,NULL,NULL,WINCOLOR);c_default(0);
    on_enter(string_list_sup);
  define(-1,10,210,180,12,0,label,"Jak vlo�it?");
  define(460,10,225,180,30,0,radio_butts,3,
                             "P�epsat v�e",
                             "P�epsat pou�it� ud�l.",
                             "P�idat na konec");c_default(1);
  define(20,10,260,180,20,0,button,"Vlo�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(30,10,285,180,20,0,button,"Zru�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_window();
  escape();
  id=o_aktual->id;
  if (id==10)
  {
	  TMULTI_ACTION **z=multi_actions[sect*4+side];
	  int cnt=str_count((TSTR_LIST)z);
	  int i;
	  
	  if (pamet) release_list((TSTR_LIST) pamet);
	  pamet=(TMULTI_ACTION **)malloc(sizeof(*pamet)*cnt);
	  memset(pamet,0,sizeof(*pamet)*cnt);
	  for (i=0;i<cnt;i++) if (z[i])
	  {
		pamet[i]=(TMULTI_ACTION *)malloc(_msize(z[i]));
		memcpy(pamet[i],z[i],_msize(z[i]));
	  }	  
  }
  if (id==20)
  {
	if (pamet)
	{
	  int i;
	  long smer=get_bit_fields(0,400,4);
	  int prim_side=f_get_value(0,445);
	  int sec_side=f_get_value(0,455);
	  long pro_prim=get_bit_fields(0,440,1);
	  long pro_sec=get_bit_fields(0,450,1);
	  int mode=f_get_value(0,460);

	  
	  for (i=0;i<maplen;i++) if (minfo[i].flags & 1)
	  {
		int j;
		for (j=0;j<4;j++)
		{
		  char set=((1<<j) & smer) || (pro_prim && mapa.sidedef[i][j].prim==prim_side) ||
			(pro_sec && mapa.sidedef[i][j].sec==sec_side);
		  if (set)
		  {
			if (mode==0)
			{
			  release_list((TSTR_LIST)multi_actions[i*4+j]);
			  multi_actions[i*4+j]=0;
			}
			else if (mode==1)
			{
			  TMULTI_ACTION **z=multi_actions[i*4+j];
			  char flags=0,eflags=0;			  
			  if (z)
			  {
				int i;
				for (i=0;i<str_count((TSTR_LIST)pamet);i++) if (pamet[i])
				{
				  flags|=pamet[i]->sound.flags;
				  eflags|=pamet[i]->sound.eflags;
				}
				for (i=0;i<str_count((TSTR_LIST)z);i++)
				{
				  TMULTI_ACTION *m=z[i];
				  if (m)
				  {
					m->sound.flags&=~flags;
					m->sound.eflags&=~eflags;
					if (m->sound.flags==0 && m->sound.eflags==0)
					{
					  free(m);
					  memcpy(z+i,z+i+1,sizeof(m)*(str_count((TSTR_LIST)z)-i-1));
					  z[str_count((TSTR_LIST)z)-1]=0;
					  --i;
					}
				  }
				}
			  }
			}
			{
			  int sect=i;
			  TMULTI_ACTION **z=multi_actions[sect*4+j];
			  int i,k=0;
			  int cnt=str_count((TSTR_LIST) z);
			  for (i=0;i<str_count((TSTR_LIST)pamet);i++) if (pamet[i])
			  {
				while (k<cnt && z[k]) k++;
				if (k==cnt)
				{
				  TMULTI_ACTION **n=(TMULTI_ACTION **)realloc(z,(cnt+32)*sizeof(TMULTI_ACTION *));
				  if (n==NULL) return;
				  memset(n+cnt,0,32*sizeof(TMULTI_ACTION *));
				  cnt+=32;
				  z=multi_actions[sect*4+j]=n;
				}
				if (z[k]==0)
				{
				  z[k]=(TMULTI_ACTION *)malloc(_msize(pamet[i]));
				  memcpy(z[k],pamet[i],_msize(pamet[i]));
				}
			  }
			}
		  }
		}
	  }
	}
  }
  close_current();
  
}

void open_multiaction_window()
  {
  if (find_window(multiaction_win)==NULL)
     {
     int i;
     CTL3D *b1;

     b1=def_border(1,0);
     for (i=1;i<maplen;i++) if (minfo[i].flags & 1) break;
     if (i==maplen) i=0;
     multiaction_win=def_window(120,SCR_WIDTH_Y - 180,"Multiakce");
     waktual->minsizx=120;
     waktual->minsizy=150;
     define(-1,5,20,1,1,0,label,"Sektor");
     define(-1,5,32,1,1,0,label,"St�na");
     define(-1,5,44,1,1,0,label,"Program:");
     define(-1,3,1,10,10,2,resizer);
     define(10,5,20,60,10,1,input_line,5,0,maplen-1,"%5d");set_default(strs(i));on_exit(test_int);
      property(NULL,NULL,flat_color(0x1f),WINCOLOR);
     define(20,5,32,60,10,1,str_line,steny2);c_default(0);on_enter(string_list_sup);
      property(NULL,NULL,flat_color(0x1f),WINCOLOR);on_control_change(mult_change_dir);
     create_multiaction_list(&mul_list,i,0);
     define(29,5,55,95,180,0,listbox,mul_list,RGB555(31,31,0),0);c_default(0);
        o_end->autoresizex=1;
        o_end->autoresizey=1;
     define(30,3,66,15,157,1,scroll_bar_v,0,10,1,SCROLLBARCOL);
        property(NULL,NULL,NULL,WINCOLOR);
        o_end->autoresizey=1;
     define(31,3,50,15,15,1,scroll_button,-1,0,"\x1e");
        property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
     define(32,3,60,15,15,2,scroll_button,1,10,"\x1f");
        property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
     define(40,5,5,50,20,3,button,"Vyma�");property(b1,NULL,NULL,WINCOLOR);on_control_change(delete_multiaction_ask);
     define(50,60,5,50,20,3,button,"Edit");property(b1,NULL,NULL,WINCOLOR);on_control_change(edit_multiaction);
     define(60,5,30,50,20,3,button,"Vlo�");property(b1,NULL,NULL,WINCOLOR);on_control_change(create_multiaction_dialoge);
     define(70,60,30,50,20,3,button,"Pam�t");property(b1,NULL,NULL,WINCOLOR);on_control_change(copy_and_paste);
     //define(70,60,30,50,20,3,button,"Po�ad�");property(b1,NULL,NULL,WINCOLOR);
     redraw_window();
     }
  select_window(wiz_tool_numb);close_current();
  }

void update_multiactions()
  {
  void *wsave;
  WINDOW *f;
  char c[20];
  int i,s;

  wsave=waktual;
  f=find_window(multiaction_win);
  if (f==NULL)
     {
     open_multiaction_window();
     return;
     }
  for (i=1;i<maplen;i++) if (minfo[i].flags & 1) break;
  if (i==maplen) i=0;
  waktual=f;
  sprintf(c,"%5d",i);
  set_value(0,10,c);
  s=f_get_value(0,20);
  create_multiaction_list(&mul_list,i,s);
  send_message(E_GUI,29,E_CONTROL,1,mul_list);
  for(i=str_count(mul_list)-1;mul_list[i]==NULL;i--);
  c_set_value(0,29,i);
  waktual=wsave;
  }

void *save_macros(long *blocksize)
  {
  long maxsize=0;
  int i,j;
  TMULTI_ACTION **q;
  char *data,*c;
  int m=0;

  for(i=1;i<maplen*4;i++) //nejprve zjistime kolik je potreba mista;
     {
     if ((q=multi_actions[i])!=NULL)
        {
        m=str_count((TSTR_LIST)q);
        maxsize+=4;
        for(j=0;j<m;j++,q++)
           if (*q!=NULL) maxsize+=4+action_sizes[(*q)->general.action];
        maxsize+=4;
        }
     }
  maxsize+=4;
  c=data=(char *)getmem(maxsize);
  for(i=1;i<maplen*4;i++) //nejprve zjistime kolik je potreba mista;
     {
     if ((q=multi_actions[i])!=NULL)
        {
        int siz;
        m=str_count((TSTR_LIST)q);
        memcpy(c,&i,4);c+=4;
        for(j=0;j<m;j++,q++)
           if (*q!=NULL)
           {
           siz=action_sizes[(*q)->general.action];
           memcpy(c,&siz,4);c+=4;
           memcpy(c,*q,siz);c+=siz;
           }
        m=0;
        memcpy(c,&m,4);c+=4;
        }
     }
  memcpy(c,&m,4);
  *blocksize=maxsize;
  return (void *)data;

  }

void load_macros(void *data)
  {
  long *c;
  int sect,sid;

  c=data;
  while (*c)
     {
     int i,siz,csiz;
     TMULTI_ACTION a;
     sect=*c>>2;
     sid=*c & 3;
     c++;i=0;
     while (*c)
        {
        siz=*c++;
        if (siz>sizeof(a)) csiz=sizeof(a);else csiz=siz;
        memcpy(&a,c,csiz);
        c=(long *)((char *)c+siz);
        add_multiaction(sect,sid,i++,&a);
        }
     c++;
     }
  }
