#include <stdio.h>
#include <mem.h>
#include <malloc.h>
#include "..\types.h"
#include "..\memman.h"
#include "..\devices.h"
#include "..\event.h"
#include "..\bmouse.h"
#include "..\bgraph.h"
#include "..\gui.h"
#include "..\basicobj.h"
#define WINCOLOR RGB555(24,24,24)
#define LABELCOLOR RGB555(0,0,15)
word *icones;
word icone_color[7]={0x2108,0x7fff,0x000f,0x4210,0x6f7b};

void close_test()
  {
  close_window(waktual);
  }

void logo(void)
  {
  CTL3D ctl;
  word *p;

  ctl.light=0x7fff;
  ctl.shadow=0x4210;
  ctl.bsize=2;
  ctl.ctldef=0;
  draw_border(120,90,0x18f,0x6a,&ctl);
  p=(word *)load_file("..\\konvert\\mapedit.hi");
  put_picture(120,90,p);
  free(p);
  showview(0,0,0,0);
  }

void *wait_ms_key(EVENT_MSG *msg)
  {
  MS_EVENT *ms;
  if (msg->msg==E_INIT) return &wait_ms_key;
  if (msg->msg==E_MOUSE)
     {
     ms=get_mouse(msg);
     if (ms->event_type & 4) terminate();
     }
  return NULL;
  }


void wait_mouse(void)
  {
  send_message(E_ADD,E_MOUSE,wait_ms_key);
  escape();
  send_message(E_DONE,E_MOUSE,wait_ms_key);
  }


void init(void)
  {
        default_font=load_file("d:\\tp\\vga\\boldcz.fon");
    icones=load_file("d:\\tp\\vga\\ikones.fon");
    if (initmode32())
     {
     word *p;

     p=load_file("..\\xlat256.pal");
     if (initmode256(p))
        initmode_lo(p);
     }
    curcolor=0x6318;memcpy(charcolors,flat_color(0x0000),sizeof(charcolors));
    init_events(100);
    curfont=default_font;
    register_ms_cursor(load_file("..\\konvert\\sipka.HI"));
    init_mysky();
    send_message(E_ADD,E_STATUS_LINE,status_line,16);
    send_message(E_STATUS_LINE,E_ADD,E_IDLE,show_time);
    send_message(E_STATUS_LINE,E_ADD,E_IDLE,status_mem_info);
    send_message(E_STATUS_LINE,E_ADD,E_IDLE,mouse_xy);
    ukaz_mysku();
    redraw_desktop();logo();wait_mouse();
    install_gui();
  }
/*
 w=create_window(100,100,400,200,0x6318,&x);
  id=desktop_add_window(w);
  define(10,20,50,30,0,sample,"Test");
  property(&x,NULL,flat_color(0x7000),0xffff);
  define(10,20,70,30,3,button,"Tlacitko");
  property(NULL,NULL,flat_color(0x000f),0x01c0);on_change(close_test);
  w=create_window(5,5,200,200,0x6318,&x);
  id=desktop_add_window(w);
  define(50,50,70,30,3,button,"Tlacitko");
  property(NULL,NULL,flat_color(0x7fff),0x000f);on_change(close_test);
  w=create_window(300,150,300,200,0x6318,&x);
  id=desktop_add_window(w);
  define(50,50,70,30,3,button,"Tlacitko");
  property(NULL,NULL,flat_color(0x7fff),0x000f);on_change(close_test);
 */

void def_window(word xs,word ys,char *name)
  {
  word x=0,y=0;
  WINDOW *p;
  CTL3D ctl;

  if (waktual!=NULL)
     {
     x=waktual->x;
     y=waktual->y;
     }

  highlight(&ctl,WINCOLOR);
  ctl.bsize=2;ctl.ctldef=0;
  x+=20;y+=20;
  if (x+xs>MAX_X-2) x=MAX_X-2-xs;
  if (y+ys>MAX_Y-2) y=MAX_Y-2-ys;
     p=create_window(x,y,xs,ys,WINCOLOR,&ctl);
     desktop_add_window(p);
  define(0,2,2,xs-25,14,0,win_label,name);
     ctl.bsize=1;ctl.ctldef=1;
     property(&ctl,default_font,flat_color(0x7fe0),LABELCOLOR);
   define(1,xs-20,1,19,16,0,button,"\x1f");
     property(NULL,icones,&icone_color,WINCOLOR);on_change(close_test);

  }

void Simple_window(void);

void open_next(void)
  {
  def_window(300,200,"Dal¨¡ okno");
  Simple_window();
  redraw_window();
  }

void Simple_window(void)
  {
  CTL3D ctl;
  highlight(&ctl,WINCOLOR);
  ctl.bsize=2;ctl.ctldef=0;

  define(10,30,30,80,20,0,button,"Nov‚ okno");
     property(NULL,default_font,flat_color(0000),WINCOLOR);
     on_change(open_next);
  define(20,120,30,80,20,0,button,"Konec");
     property(NULL,default_font,flat_color(0000),WINCOLOR);
     on_change(terminate);
  define(30,60,60,90,12,0,check_box,"Check box");
     property(NULL,default_font,flat_color(0),WINCOLOR);c_default(0);
  define(40,60,80,4*15,4*15,0,radio_butts,4,"Test 1","Test 2","Test 3","Test 4");
     property(NULL,default_font,flat_color(0),WINCOLOR);c_default(1);
  define(50,170,80,80,20,0,toggle_button,"Toggle");c_default(1);
     property(NULL,default_font,flat_color(0),WINCOLOR);
  define(60,20,25,180,10,3,input_line,255);
   property(&ctl,default_font,flat_color(0),WINCOLOR);set_default("Vstupn¡ linka");


  }


void main()
  {
  init();
  def_window(300,200,"Prvn¡ okno");
  Simple_window();
  redraw_desktop();
  escape();
//  deinstall_mouse_handler();
  closemode();
  }




