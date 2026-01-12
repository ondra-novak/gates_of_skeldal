#include "types.h"
#include <stdio.h>
#include <i86.h>
#include <stdlib.h>
#include "devices.h"
#include "event.h"
#include "bgraph.h"
#include "bmouse.h"
#include "memman.h"
#define E_TEST 200

void gettest()
  {

  register_ms_cursor(load_file("sipka.HI"));
  show_ms_cursor(0,0);
//  install_mouse_handler();
  hranice_mysky(0,0,639,399);
  while (!(ms_basic_info.mouse_code & 0xfffe))
     {
     if (ms_basic_info.mouse_event)
        move_ms_cursor(ms_basic_info.mouse_cx,ms_basic_info.mouse_dx);
     }
  }


void modetest()
  {
  word y;
  curcolor=0x7fff;
   for(y=0;y<400;y+=10)
     {
     line(639-y,399,639,y);
     line(639-y,0,639,399-y);
     line(y,0,0,399-y);
     line(y,399,0,y);

     }
  curcolor=0x6318;
     bar32(160,100,480,300);

  curcolor=0x7fff;
     hor_line32(160,100,480);
     hor_line32(161,101,479);
     ver_line32(160,101,300);
     ver_line32(161,102,299);
  curcolor=0x4210;
     hor_line32(160,300,480);
     hor_line32(161,299,479);
     ver_line32(480,100,300);
     ver_line32(479,101,299);
fontdsize=0;
position(165,105);outtext("TEXT");

   }

void pic_test(word x,word y,char *filename)
  {
  void *p;

  p=load_file(filename);
  if (p==NULL) return;
  put_picture(x,y,p);
  free(p);
  }


void main_x()
  {
  if ((palmem=load_file("xlat256.pal"))==NULL) return;
//  initmode_lo (palmem);
initmode32();
  curfont=load_file("d:\\tp\\vga\\boldcz.fon");
  if (curfont==NULL) return;
  pic_test(0,0,"ADD11.HI");
  showview(0,0,0,0);getchar();
  modetest();
  showview(0,0,0,0);
    }

void *test1(EVENT_MSG *a,void *b)
  {
  MS_EVENT *ms;

  b;
  if (a->msg==E_INIT) return &test1;
  ms=get_mouse(a);
  exit_wait=ms->tl1;
  return NULL;
  }

void *test2(EVENT_MSG *a,void *b)
  {
  b;
  if (a->msg==E_INIT) return &test2;
  exit_wait=1;
  return NULL;
  }

void main()
  {
  init_events(20);
  main_x();
  init_mysky();
  register_ms_cursor(load_file("sipka.HI"));
  ukaz_mysku();
  send_message(E_ADD,E_MOUSE,&test1);
  send_message(E_ADD,E_KEYBOARD,&test2);
  escape();
  send_message(E_DONE,E_MOUSE,&test1);
  send_message(E_DONE,E_KEYBOARD,&test2);
  closemode();
//  deinstall_mouse_handler();
  }




