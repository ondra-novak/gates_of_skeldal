#include <types.h>
#include <vesa.h>
#include <mem.h>
#include <stdio.h>
#include <bgraph.h>
#include <bmouse.h>
#include <devices.h>
#include <gui.h>
#include <basicobj.h>
#include <strlite.h>
#include <strlists.h>
#include "setup.h"

static void *xlat256=NULL;
static void *xlat16=NULL;

static void create_xlats()
  {
  if (xlat256==NULL) xlat256=create_special_palette();
  if (xlat16==NULL) xlat16=create_blw_palette16();
  }


static void initgr_common()
  {
  register_ms_cursor(&sipka);
  init_mysky();
  hranice_mysky(0,0,639,479);
  update_mysky();
  schovej_mysku();
  bar(0,0,639,479);
  showview(0,0,0,0);
  }

int initgr_auto()
  {
  int vmode;
  create_xlats();
  vmode=2;
  if (initmode32())
     {
     vmode=1;
     if (initmode256(xlat256))
        {
        vmode=0;
        initmode16(xlat16);
        }
     }
  initgr_common();
  return vmode;
  }

int initgr_spec(int vmode)
  {
  int i=-1;
  create_xlats();
  switch (vmode)
     {
     case 0:i=initmode_lo(xlat256);break;
     case 1:i=initmode256(xlat256);break;
     case 2:i=initmode32();break;
     }
  initgr_common();
  return i;
  }

void initgr_low()
  {
  create_xlats();
  initmode16(xlat16);
  initgr_common();
  }

void donegr()
  {
  closemode();
  done_mysky();
  }

void kresli_okno(WINDOW *w)
  {
  int x,y;
  int xs,ys,xsr,ysr;
  int x1,y1,xs1,ys1;
  int i,j;

  xsr=ramecky[0][0];
  ysr=ramecky[0][1];
  x1=w->x-12;
  y1=w->y-12;
  xs1=w->xs+24;
  ys1=w->ys+24;
  xs=xs1/xsr;
  ys=ys1/ysr;
  curcolor=w->color;
  bar(w->x,w->y,w->x+w->xs,w->y+w->ys);
  for(j=0,y=y1;j<ys;j++,y+=ysr)
     if (j==0)
        for(i=0,x=x1;i<xs;i++,x+=xsr)
           if (i==0) put_picture(x,y,ramecky[0]);
           else if (i+1==xs) put_picture(x,y,ramecky[2]);
           else put_picture(x,y,ramecky[1]);
     else if (j+1==ys)
        for(i=0,x=x1;i<xs;i++,x+=xsr)
           if (i==0) put_picture(x,y,ramecky[5]);
           else if (i+1==xs) put_picture(x,y,ramecky[5]);
           else put_picture(x,y,ramecky[6]);
     else
        {
        x=x1;
        put_picture(x+xsr*xs-xsr,y,ramecky[4]);
        put_picture(x,y,ramecky[3]);
        }
  }

long def_window(word xs,word ys,char *name)
  {
  word x=0,y=0;
  WINDOW *p;
  CTL3D ctl;
  FC_TABLE fc;
  long q;

  if (waktual!=NULL)
     {
     x=waktual->x;
     y=waktual->y;
     }
  name;
  highlight(&ctl,WINCOLOR);
  ctl.bsize=2;ctl.ctldef=0;
  x+=20;y+=20;
  memcpy(fc,flat_color(0x7fe0),sizeof(FC_TABLE));
  fc[0]=0x0000;
  if (x+xs>MAX_X-2) x=MAX_X-2-xs;
  if (y+ys>MAX_Y-2) y=MAX_Y-2-ys;
     p=create_window(x,y,xs,ys,WINCOLOR,&ctl);
     q=desktop_add_window(p);
  define(0,2,2,xs-5-20*(xs>=70),14,0,win_label,name);
     ctl.bsize=1;ctl.ctldef=1;
      o_end->autoresizex=1;
     property(&ctl,vga_font,&fc,LABELCOLOR);
  if (xs>=70)
     {
  define(1,1,1,19,16,1,button,"\x0f");
     property(NULL,icones,&icone_color,WINCOLOR);on_control_change(close_current);
     }
  return q;
  }

int def_dialoge(word x,word y,word xs, word ys, char *name,char modal)
  {
  CTL3D ctl;
  FC_TABLE fc;
  WINDOW *p;
  int i;

  memcpy(fc,flat_color(0x7fe0),sizeof(FC_TABLE));
  if (modal & 0x2)
     {
     ctl.bsize=12;
     xs=((xs+6)/12)*12;
     ys=((ys+6)/12)*12;
     }
  else
     memcpy(&ctl,def_border(2,WINCOLOR),sizeof(CTL3D));
  p=create_window(x,y,xs,ys,WINCOLOR,&ctl);
  i=desktop_add_window(p);
  if (modal & 1) set_window_modal();
  memcpy(&ctl,def_border(5,WINCOLOR),sizeof(CTL3D));
  if (name!=NULL)
     {
     define(0,2,2,xs-4,14,0,win_label,name);
     o_end->autoresizex=1;
     property(&ctl,vga_font,&fc,LABELCOLOR);
     }
  if (modal & 0x2) p->draw_event=kresli_okno;
  return i;
  }

void def_listbox(int id,word x,word y,word xs,word ys,TSTR_LIST ls,int ofs,int color)
  {
  CTL3D b1,b2;
  word black[]={0,0,0,0,0,0};

  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  define(id+1,x+xs+4,y+18,15,ys-35,0,scroll_bar_v,0,10,1,0x0200);
  property(&b2,NULL,NULL,WINCOLOR);
  define(id+2,x+xs+4,y,14,14,0,scroll_button,-1,0,"\x4");
  property(&b1,icones,black,WINCOLOR);on_control_change(scroll_support);
  define(id+3,x+xs+4,y+ys-14,14,14,0,scroll_button,1,10,"\6");
  property(&b1,icones,black,WINCOLOR);on_control_change(scroll_support);
  define(id,x,y,xs,ys,0,listbox,ls,color,ofs);
  property(&b2,NULL,NULL,WINCOLOR);
  }

