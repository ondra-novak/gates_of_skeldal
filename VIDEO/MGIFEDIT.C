#include <libs/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <malloc.h>
#include <libs/memman.h>
#include <libs/event.h>
#include <libs/bmouse.h>
#include <libs/gui.h>
#include <libs/basicobj.h>
#include <libs/zvuk.h>
#include <libs/bgraph.h>
#include "lzw.h"
#include "mgifeobj.h"
#include "mgifeact.h"
#include "mgifedit.h"
#include "mgifebld.h"
#include <strlists.h>
#include <libs/strlite.h>
#include <direct.h>
#include <libs/mgfplay.h>
#include <libs/wav.h>

void *vga_font=&boldcz;
void *icones=&ikones;
void *ms_cur=&sipka;
word icone_color[7]={0x2108,0x7fff,0x000f,0x4210,0x6f7b};
int win_preview;
int win_program=-1;
int cur_track=-1;
int win_control;
TSTR_LIST smp_list;
char last_path[256]="";
char *prj_name;

static void grinit(void)
  {
  initmode32();
  register_ms_cursor(ms_cur);
  init_mysky();
//  hranice_mysky(0,0,639,479);
  update_mysky();
  schovej_mysku();
  bar(0,0,639,479);
  showview(0,0,0,0);
  }


void init_sound()
  {
  int a,b,c,d;
  if (sound_detect(&a,&b,&c,&d))
     {
     puts("No sound device found");
     a=DEV_NOSOUND;
     }
  set_mixing_device(a,22000,b,c,d);
  }

static void f10exit()
  {
  int c;
  do
     {
     c=*(int*)task_wait_event(E_KEYBOARD);
     if ((c & 0xff)==0 && (c>>8)=='D') terminate_gui();
     if ((c & 0xff)==0 && (c>>8)=='C')
        {
        done_mysky();
        closemode();
        grinit();
        redraw_desktop();
        schovej_mysku();
        zobraz_mysku();
        }
     }
  while (1);
  }

void initialization(void)
  {
  init_sound();
  grinit();
  init_events();
  install_gui();
  zobraz_mysku();
  update_mysky();
  add_task(1024,f10exit);
  redraw_desktop();do_events();
  msg_box_font=vga_font;
  msg_icn_font=icones;
  getcwd(last_path,255);
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
  memcpy(&ctl,def_border(2,WINCOLOR),sizeof(CTL3D));
  p=create_window(x,y,xs,ys,WINCOLOR,&ctl);
  i=desktop_add_window(p);
  if (modal) set_window_modal();
  ctl.bsize=1;ctl.ctldef=1;
  define(0,1,1,xs-2,14,0,win_label,name);
  o_end->autoresizex=1;
  property(&ctl,vga_font,&fc,LABELCOLOR);
  return i;
  }


static void display_progress(void)
  {
  def_dialoge(10,300,620,80,"Na��t�m informace o filmu...",1);
  define(10,10,20,600,30,0,done_bar,100);
  property(def_border(3,WINCOLOR),NULL,flat_color(0x2e0),WINCOLOR);
  define(30,5,5,80,20,2,button,"Konec");on_control_change(terminate_gui);
   property(def_border(1,0),vga_font,flat_color(0),BUTTONCOLOR);
  c_default(0);
  redraw_window();
  exit_wait=0;
  }

static char show_progress(int perc)
  {
  c_set_value(0,10,perc);
  do_events();
  return exit_wait;
  }

static void done_progress(void)
  {
  if (exit_wait)
     {
     shutdown();
     puts("Program aborted by user...");
     exit(0);
     }
  exit_wait=0;
  close_current();
  redraw_desktop();
  }

void show_frame(int frame)
  {
  static int last_task=0;

  if (last_task) term_task(last_task);
  while (last_task!=0) task_sleep(NULL);
  if (frame>=0) last_task=add_task(32768,build_frame,frame,&last_task);
  }

int delay_task=0;
int delay_task_counter=0;

void delayed_redraw() //task!
  {
  for(delay_task_counter=0;delay_task_counter<5;delay_task_counter++) task_wait_event(E_TIMER);
  delay_task=0;
  redraw_desktop();
  }

static void change_frame_pos()
  {
  int frame;
  frame=f_get_value(win_preview,20);
  if (find_window(win_program)!=NULL)
     {
     set_value(win_program,10,&frame);
     set_value(win_program,20,&frame);
     set_value(win_program,30,&frame);
     if (!delay_task) delay_task=add_task(1024,delayed_redraw);
     else delay_task_counter=0;
     }
   show_frame(frame);
  }

void init_desktop(void)
  {
  FC_TABLE cl;

  cl[0]=0;cl[1]=0x610;
  win_preview=def_dialoge(10,10,324,228,"Preview",0);
  define(5,1,1,14,14,1,color_box);c_default(LABELCOLOR);
  define(10,2,20,320,180,0,pic_viewer);c_default(0);
  define(20,26,4,271,17,3,scroll_bar_h,0,total_frames-1,total_frames/16,0x0200);
  property(def_border(3,WINCOLOR),NULL,NULL,WINCOLOR);c_default(0);on_control_change(change_frame_pos);
  define(21,2,4,21,19,2,scroll_button,1,100,"\x1c");
  property(NULL,icones,&cl,WINCOLOR);on_control_change(scroll_support);
  define(22,2,4,21,19,3,scroll_button,-1,-100,"\x1d");
  property(NULL,icones,&cl,WINCOLOR);on_control_change(scroll_support);
  redraw_desktop();
  show_frame(0);
  }


void shutdown()
  {
  done_mysky();
  closemode();
  }


static void init_editor()
  {
  char *chyba;
  display_progress();
  switch(examine_mgif_file(show_progress))
     {
     case EX_NOT_FOUND:chyba="MGIF soubor nenalezen!";break;
     case EX_NO_SOUND:chyba="Film nem� zvukovou stopu. Sestav jej se zvukem.";break;
     case EX_READ_ERROR:chyba="Chyba p�i �ten�. Film je mo�n� po�kozen";break;
     case EX_SOUND_ERROR:chyba="Nekonzistentn� zvukov� stopa!";break;
     default:chyba=NULL;
     }
  if (chyba!=NULL)
     {
     msg_box(PRG_HEADER,'\x1',chyba,"Stop",NULL);
     shutdown();
     abort();
     }
  done_progress();
  }

static int step_vpoint=1;

static void change_vpoint()
  {
  TRACK_DATA_T **p,z;
  int id=o_aktual->id;
  int frame;
  int m;

  if (id>100)
     {
     p=&smp_prg[cur_track].pravy;
     id-=100;
     }
  else
     p=&smp_prg[cur_track].levy;
  frame=f_get_value(win_preview,20);
  get_vpoint(*p,frame,&z);
  m=z.vpoint1;
  switch(id)
     {
     case 50:m-=step_vpoint>>3;if (m<0) m=0; set_vpoint(p,frame,m,m);step_vpoint++;break;
     case 40:m+=step_vpoint>>3;if (m>255) m=255; set_vpoint(p,frame,m,m);step_vpoint++;break;
     case 60:delete_vpoint(p,frame);break;
     case 70:if (z.next!=NULL) set_vpoint(p,frame,z.next->vpoint1,z.next->vpoint1);break;
     case 80:change_vpoint_spacing(*p,frame,step_vpoint>>3);step_vpoint++;break;
     case 90:change_vpoint_spacing(*p,frame,-(step_vpoint>>3));step_vpoint++;break;
     }
  redraw_window();
  }

static void step_clear(EVENT_MSG *msg)
  {
  if (msg->msg==E_LOST_FOCUS)
     {
     step_vpoint=8;
     return;
     }
  if (msg->msg==E_MOUSE)
     {
     MS_EVENT *ms;

     ms=get_mouse(msg);
     if (ms->event_type & (0x4|0x8)) step_vpoint=8;
     }
  }

static void toggle_restart()
  {
  int *p;
  int i,c;
  int frame;

  frame=f_get_value(win_preview,20);
  c=smp_prg[cur_track].starts_count;
  p=smp_prg[cur_track].starts;
  for(i=0;i<c;i++) if (frame==p[i]) break;
  if (i==c) add_restart(smp_prg+cur_track,frame);
  else delete_restart(smp_prg+cur_track,frame);
  compare_vol_table(smp_prg+cur_track);
  set_vol_table_nul();
  compare_vol_table(smp_prg+cur_track);
  redraw_window();
  }

static void max_min_mid_align()
  {
  int frame;
  TRACK_DATA_T z1,**p1;
  TRACK_DATA_T z2,**p2;
  int vol1,vol2,vol;


  frame=f_get_value(win_preview,20);
  p1=&smp_prg[cur_track].levy;
  p2=&smp_prg[cur_track].pravy;
  get_vpoint(*p1,frame,&z1);
  get_vpoint(*p2,frame,&z2);
  vol1=z1.vpoint1;
  vol2=z2.vpoint1;
  switch (o_aktual->id)
     {
     case 210:vol=max(vol1,vol2);break;
     case 220:vol=min(vol1,vol2);break;
     case 230:vol=(vol1+vol2)/2;break;
     case 240:vol=vol2;break;
     case 250:vol=vol1;break;
     }
  set_vpoint(p1,frame,vol,vol);
  set_vpoint(p2,frame,vol,vol);
  redraw_window();
  }

static void change_frame_clk1()
  {
  int fr;

  fr=f_get_value(0,o_aktual->id);
  c_set_value(win_preview,20,fr);
  c_set_value(win_program,10,fr);
  c_set_value(win_program,20,fr);
  c_set_value(win_program,30,fr);
  show_frame(fr);
  }


static void mute_channel()
  {
  char c;
  int i;

  c=f_get_value(0,260);
  for(i=10;i<260;i++)set_enable(0,i,!c);
  if (smp_prg[cur_track].muted!=c)
     {
     compare_vol_table(smp_prg+cur_track);
     set_vol_table_nul();
     compare_vol_table(smp_prg+cur_track);
     smp_prg[cur_track].muted=c;
     }
  }

void open_program_window(int track)
  {
  FC_TABLE cl1,cl2;
  CTL3D *cl;
  int i;
  int frame;

  frame=f_get_value(win_preview,20);
  if (track>=samples_total) track=-1;
  if (cur_track!=track)
     {
     WINDOW *w;
     w=find_window(win_program);if (w!=NULL) close_window(w);
     if (cur_track>=0 && cur_track<samples_total)
           compare_vol_table(smp_prg+cur_track);
     cur_track=track;
     if (cur_track>=0 && cur_track<samples_total)
           read_vol_table(smp_prg+cur_track);
     }
  else return;
  if (track<0) return;
  cl1[0]=0x8000;cl1[1]=0xf;
  memcpy(cl2,flat_color(0),sizeof(cl2));
  win_program=def_dialoge(10,250,620,220,smp_prg[track].sample_name,0);
  define(10,10,20,500,70,0,track_view,&smp_prg[track].levy);on_control_change(change_frame_clk1);
   property(def_border(5,WINCOLOR),NULL,NULL,WINCOLOR);c_default(frame);
  define(20,10,120,500,70,0,track_view,&smp_prg[track].pravy);on_control_change(change_frame_clk1);
   property(def_border(5,WINCOLOR),NULL,NULL,WINCOLOR);c_default(frame);
  define(30,10,95,500,20,0,starts_view,smp_prg+track);
   property(def_border(5,WINCOLOR),NULL,NULL,WINCOLOR);c_default(frame);
  for(i=0;i<=100;i+=100)
     {
     int y=i;
     define(40+i,85,20+y,20,35,1,scroll_button,-1,0,"\x1e");
       property(NULL,icones,&cl1,WINCOLOR);on_control_change(change_vpoint);on_event(step_clear);
     define(50+i,85,56+y,20,35,1,scroll_button,1,255,"\x1f");
       property(NULL,icones,&cl1,WINCOLOR);on_control_change(change_vpoint);on_event(step_clear);
     define(60+i,10,20+y,20,70,1,button,"X");on_control_change(change_vpoint);
       property(NULL,vga_font,cl2,WINCOLOR);
     define(70+i,32,20+y,50,20,1,button,"+���+");on_control_change(change_vpoint);
       property(NULL,vga_font,cl2,WINCOLOR);
     define(80+i,32,45+y,50,20,1,scroll_button,-1,0,"+--+");
       property(NULL,vga_font,&cl2,WINCOLOR);on_control_change(change_vpoint);on_event(step_clear);
     define(90+i,32,70+y,50,20,1,scroll_button,1,255,"+--+");
       property(NULL,vga_font,&cl2,WINCOLOR);on_control_change(change_vpoint);on_event(step_clear);
     }
   define(200,10,95,95,20,1,button,"(re)Start");property(NULL,vga_font,&cl2,WINCOLOR);
   on_control_change(toggle_restart);
   cl=def_border(1,0);
   define(210,10,5,80,20,3,button,"Max");property(cl,vga_font,&cl2,BUTTONCOLOR);on_control_change(max_min_mid_align);
   define(220,100,5,80,20,3,button,"Min");property(cl,vga_font,&cl2,BUTTONCOLOR);on_control_change(max_min_mid_align);
   define(230,190,5,80,20,3,button,"St�ed");property(cl,vga_font,&cl2,BUTTONCOLOR);on_control_change(max_min_mid_align);
   define(240,280,5,80,20,3,button,"Jako doln�");property(cl,vga_font,&cl2,BUTTONCOLOR);on_control_change(max_min_mid_align);
   define(250,370,5,80,20,3,button,"Jako horn�");property(cl,vga_font,&cl2,BUTTONCOLOR);on_control_change(max_min_mid_align);
   define(260,10,10,80,10,2,check_box,"Vypnut");on_control_change(mute_channel);c_default(smp_prg[cur_track].muted);
   redraw_window();
   mute_channel();
 }

static void change_dir()
  {
  int i;
  char *mask;
  TSTR_LIST ls;
  get_value(0,20,last_path);
  c_set_value(0,9,0);
  send_message(E_GUI,9,E_CONTROL,2);
  i=strlen(last_path);
  strupper(last_path);
  if (i!=0 && last_path[i-1]!='\\')
     {
     strcat(last_path,"\\");
     set_value(0,20,last_path);
     }
  concat(mask,last_path,"*.WAV");
  send_message(E_GUI,9,E_CONTROL,0,&ls);
  release_list(ls);
  ls=read_directory(mask,DIR_NAMES,_A_NORMAL);
  if (ls==NULL)
     {
     ls=create_list(2);
     str_add(&ls,"<nic>");
     }
  send_message(E_GUI,9,E_CONTROL,1,ls);
  }

static void dopln_jmeno_samplu()
  {
  char *name,*c;
  TSTR_LIST ls;
  int i;
  FILE *f;

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  i=f_get_value(0,9);
  if (ls[i]==NULL) return;
  c=ls[i];
  if (!strcmp(c,"<nic>")) return;
  concat(name,last_path,c);
  set_value(0,30,name);
  f=fopen(name,"rb");
  if (f!=NULL)
     {
     if (find_chunk(f,WAV_DATA)!=-1)
        {
        char d[50];

        set_value(0,60,itoa(get_chunk_size(f),d,10));
        }
     fclose(f);
     }
  }

void new_edit_sample(int track)
  {
  TSTR_LIST dir;
  CTL3D b1,b2,b3;
  char *c,empty[]="";
  char num[200];

  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  def_dialoge(20,140,600,200,"Vlo� nov� zvuk",1);
  dir=create_list(2);str_add(&dir,"<wait>");
  define(9,10,20,120,166,0,listbox,dir,0x7fff,0);on_control_change(dopln_jmeno_samplu);
  property(&b3,NULL,NULL,WINCOLOR);c_default(0);
  define(10,137,40,19,127,0,scroll_bar_v,0,10,1,0x0200);
  property(&b2,NULL,NULL,WINCOLOR);
  define(11,136,20,21,17,0,scroll_button,-1,0,"\x1e");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(12,136,170,21,17,0,scroll_button,1,10,"\x1f");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(20,170,20,420,12,0,input_line,255);set_default(last_path);on_control_exit(change_dir);
  property(&b3,vga_font,NULL,WINCOLOR);
  define(-1,170,35,1,1,0,label,"Cesta a jm�no zvuku");
  define(-1,170,65,1,1,0,label,"Jm�no stopy (voliteln�)");
  define(-1,170,95,1,1,0,label,"Za��tek opakov�n� (-1 vyp)");
  define(-1,170,125,1,1,0,label,"Konec opakov�n�");
  c=(smp_prg[track].sample_name);if (c==NULL) c=empty;
  define(30,170,50,420,12,0,input_line,255);set_default(c);
  property(&b3,vga_font,NULL,WINCOLOR);
  c=(smp_prg[track].user_name);if (c==NULL) c=empty;
  define(40,170,80,220,12,0,input_line,255);set_default(c);
  property(&b3,vga_font,NULL,WINCOLOR);
  define(50,290,110,100,12,0,input_line,199);set_default(itoa(smp_prg[track].loop_start,num,10));
  property(&b3,vga_font,NULL,WINCOLOR);
  define(60,290,140,100,12,0,input_line,199);set_default(itoa(smp_prg[track].loop_end,num,10));
  property(&b3,vga_font,NULL,WINCOLOR);
  define(100,10,10,80,20,2,button,"Storno");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  define(110,10,35,80,20,2,button,"Ok");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  define(120,10,70,38,20,2,button,"");property(&b1,vga_font,NULL,BUTTONCOLOR);
  define(130,52,70,38,20,2,button,"");property(&b1,vga_font,NULL,BUTTONCOLOR);
  redraw_window();
  change_dir();
  escape();
  if (o_aktual->id==110)
     {
     char s[256];
     TRACK_INFO_T *t=smp_prg+track;

     get_value(0,30,s);free(t->sample_name);t->sample_name=NewArr(char,strlen(s)+1);
     strcpy(t->sample_name,s);
     get_value(0,40,s);free(t->user_name);
     if (s[0]!=0)
        {
        t->user_name=NewArr(char,strlen(s)+1);
        strcpy(t->user_name,s);
        }
     else t->user_name=NULL;
     get_value(0,50,s);t->loop_start=atoi(s);
     get_value(0,60,s);t->loop_end=atoi(s);
     }
  close_current();
  }

static void select_sample()
  {
  int i;
  i=f_get_value(0,9);
  open_program_window(i);
  }

static void update_sample_list()
  {
  TSTR_LIST ls;
  int i;
  char p;

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  build_sample_list(&ls);
  send_message(E_GUI,9,E_CONTROL,1,ls);
  i=f_get_value(0,9);
  if (i>=samples_total) i=samples_total-1;
  if (i<0) i=0;
  c_set_value(0,9,i);
  p=samples_total!=0;
  set_enable(0,30,p);set_enable(0,40,p);set_enable(0,50,p);
  run_background(select_sample);
  }


static void new_sample()
  {
  int track;
  track=add_sample("");
  open_program_window(-1);
  select_sample();
  new_edit_sample(track);
  select_window(win_control);
  if (smp_prg[track].sample_name[0]==0) remove_sample(track);
  else
     {
     c_set_value(0,9,track);
     set_vol_table_nul();
     compare_vol_table(smp_prg+track);
     }
  update_sample_list();
  }

static void edit_sample()
  {
  int i;
  i=f_get_value(0,9);
  if (i>=samples_total || i<0) return;
  new_edit_sample(i);
  compare_vol_table(smp_prg+cur_track);
  set_vol_table_nul();
  compare_vol_table(smp_prg+cur_track);
  open_program_window(-1);
  update_sample_list();
  }

static void delete_sample()
  {
  int i;
  i=f_get_value(0,9);
  if (i>=samples_total || i<0) return;
  if (msg_box("Ot�zka?",'\x2',"Odstranit zvuk?","Ano","Ne",NULL)==2) return;
  open_program_window(-1);
  remove_sample(i);
  update_sample_list();
  }

static void duplic_sample()
  {
  int track;
  int i;
  TRACK_INFO_T *t1,*t2;
  i=f_get_value(0,9);
  if (i>=samples_total || i<0) return;
  t1=smp_prg+i;
  track=add_sample(t1->sample_name);
  open_program_window(-1);
  t1=smp_prg+i;
  t2=smp_prg+track;
  if (t1->user_name!=NULL)
     {
     t2->user_name=NewArr(char,strlen(t1->user_name)+1);
     strcpy(t2->user_name,t1->user_name);
     }
  else t2->user_name=NULL;
  t1->loop_start=t2->loop_start;
  t1->loop_end=t2->loop_end;
  c_set_value(0,9,track);
  send_message(E_GUI,9,E_CONTROL,2);
  update_sample_list();
  }

static void prehrat_cele()
  {
  show_frame(-1);
  def_dialoge(2,42,635,378,"P�ehr�t cel�",1);
  redraw_window();
  curcolor=0;
  start_mixing();
  play_animation(mgif_filename,SMD_HICOLOR,60,1);
  stop_mixing();
  close_current();
  }

static void stop_preview_clk()
  {
  stop_preview();
  }

static void prehrat_preview()
  {
  OBJREC *o;
  WINDOW *w;
  int x,y;

  w=find_window(win_preview);
  o=find_object(w,10);
  show_frame(-1);
  x=o->locx+320+4;
  y=o->locy+180+4;
  if (x>500) x=2;if (y>400) y=2;
  def_dialoge(x,y,100,80,"N�hled",1);
  define(10,10,25,80,30,0,button,"STOP");
   property(def_border(1,0),vga_font,NULL,BUTTONCOLOR);on_control_change(stop_preview_clk);
  redraw_window();
  preview_block(f_get_value(win_preview,20),o->locx,o->locy);
  close_current();
  }

static void make()
  {
  if (cur_track>=0 && cur_track<samples_total)
           compare_vol_table(smp_prg+cur_track);
  def_dialoge(10,300,620,80,"Sestavuji...",1);
  define(10,10,20,600,30,0,done_bar,100);c_default(0);
  property(def_border(3,WINCOLOR),NULL,flat_color(0x2e0),WINCOLOR);
  define(20,5,5,480,12,3,input_line,100);set_default("Ukl�d�m projekt....");
  define(30,5,5,80,20,2,button,"Stop");on_control_change(terminate_gui);
   property(def_border(1,0),vga_font,NULL,BUTTONCOLOR);
  redraw_window();
  save_project(prj_name);
  build_all_samples();
  close_current();
  }

static void build()
  {
  int i;

  if (msg_box(PRG_HEADER,'\x2',"Chce� spustit REBUILD filmu? Tato funkce sestav� CELOU zvukovou stopu, co� m��e zabrat dost �asu","Ano","Ne",NULL)==2) return;
  for(i=0;i<total_frames;i++)
     mgf_frames[i].changed=1;
  make();
  }

static void exit_prog()
  {
  int i;
  i=msg_box("Exit",'\x2',"Ulo�it projekt p�ed ukon�en�m programu?","Ano","Ne","Storno",NULL);
  switch (i)
     {
     case 3:return;
     case 1:save_project(prj_name);
     case 2:terminate_gui();
     }
  }

static int calcul_build_memory()
  {
  int suma=0;
  int i;

  for(i=0;i<total_frames;i++) suma+=mgf_frames[i].track_size*2;
  suma/=1024;
  return suma;
  }

static void info()
  {
  static char *texty[]=
     {
     "Film:",
     "Project:",
     "Celkem sn�mk�:",
     "Pr�zdn�ch sn�mk�:",
     "Vyu�it� pam��(KB):",
     "Sestavovac� pam��(KB):",
     "Stop:",
     "Sestaveno sn�mk�:",
     "Cas sestaven�:",
     };
  char str[200];char *c;
  int i;

  def_dialoge(150,100,340,240,"Info",1);
  for(i=0;i<9;i++)
     define(-10,180-text_width(texty[i]),20+i*15,1,1,0,label,texty[i]);
  define(-10,50,170,1,1,0,label,"Amplifikace:");
  c=strrchr(mgif_filename,'\\');if (c==NULL)c=mgif_filename;else c++;
  define(-10,190,20,1,1,0,label,c);
  c=strrchr(prj_name,'\\');if (c==NULL)c=prj_name;else c++;
  define(-10,190,35,1,1,0,label,c);
  define(-10,190,50,1,1,0,label,itoa(total_frames,str,10));
  define(-10,190,65,1,1,0,label,itoa(frame_shift,str,10));
  define(-10,190,80,1,1,0,label,itoa(total_frames*sizeof(FRAME_DEFS_T)/1024,str,10));
  define(-10,190,95,1,1,0,label,itoa(calcul_build_memory(),str,10));
  define(-10,190,110,1,1,0,label,itoa(samples_total,str,10));
  define(-10,190,125,1,1,0,label,itoa(frames_build,str,10));
  i=get_building_time(); sprintf(str,"%02d:%02d",i/60,i%60);
  define(-10,190,140,1,1,0,label,str);
  define(10,190,170,40,60,0,radio_butts,5,
                                       "1/1",
                                       "1/2",
                                       "1/4",
                                       "1/8",
                                       "1/16");c_default(amplifikace);
  define(100,5,5,60,20,2,button,"Ok");on_control_change(terminate_gui);
  redraw_window();
  escape();
  if (amplifikace!=f_get_value(0,10))
     {
     msg_box(PRG_HEADER,'\x1',"Aby tato zm�na m�la spr�vn� efekt, mus�� prov�st BUILD!","Ok",NULL);
     amplifikace=f_get_value(0,10);
     }
  close_current();
  }

char how_frames(char *prompt,int *value)
  {
  char buffer[20];
  CTL3D b1;

  def_dialoge(200,200,200,100,"GINSERT/GDELETE",1);
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  define(10,5,20,1,1,0,label,prompt);
  define(20,10,35,60,12,1,input_line,10);set_default(itoa(*value,buffer,10));
   property(def_border(5,WINCOLOR),NULL,NULL,0x7fff);
  define(30,5,5,80,20,2,button,"Ok");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  define(40,90,5,80,20,2,button,"Zru�it");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  redraw_window();
  chyba:
  escape();
  if (o_aktual->id==30)
     {
     get_value(0,20,buffer);
     if (sscanf(buffer,"%d",value)!=1)
        {
        msg_box(PRG_HEADER,'\x1',"To mus� b�t ��slo!","Ok",NULL);
        goto chyba;
        }
     close_current();
     return 1;
     }
  close_current();
  return 0;
  }

static void ginsert()
  {
  int v;
  int frame;

  frame=f_get_value(win_preview,20);
  v=get_inserted_frames(frame);
  if (how_frames("Kolik bylo vlo�eno?",&v)==0) return;
  insert_global(frame,v);
  redraw_desktop();
  }

static void gdelete()
  {
  int v;
  int frame;

  frame=f_get_value(win_preview,20);
  v=get_deleted_frames();
  if (how_frames("Kolik bylo vymaz�no?",&v)==0) return;
  delete_global(frame,v);
  redraw_desktop();
  }



void control_window()
  {
  CTL3D b1,b2,b3;
  char p;

  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  win_control=def_dialoge(350,10,280,228,PRG_HEADER,0);
  build_sample_list(&smp_list);
  define(9,10,20,100,196,0,listbox,smp_list,0x7fff,0);on_control_change(select_sample);
  property(&b3,NULL,NULL,WINCOLOR);c_default(-1);
  define(10,117,40,19,157,0,scroll_bar_v,0,10,1,0x0200);
  property(&b2,NULL,NULL,WINCOLOR);
  define(11,116,20,21,17,0,scroll_button,-1,0,"\x1e");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(12,116,200,21,17,0,scroll_button,1,10,"\x1f");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(20,140,20,65,20,0,button,"Nov�");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(new_sample);
  define(30,210,20,65,20,0,button,"Odstra�");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(delete_sample);
  define(40,140,45,65,20,0,button,"Oprav");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(edit_sample);
  define(50,210,45,65,20,0,button,"Duplikuj");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(duplic_sample);
  define(60,140,120,65,20,0,button,"P�ehr�t");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(prehrat_cele);
  define(70,210,120,65,20,0,button,"N�hled");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(prehrat_preview);
  define(80,140,145,65,20,0,button,"Make");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(make);
  define(90,210,145,65,20,0,button,"Build");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(build);
  define(100,210,10,65,20,3,button,"Exit");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(exit_prog);
  define(110,140,10,65,20,3,button,"Info");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(info);
  define(120,140,170,65,20,0,button,"GInsert");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(ginsert);
  define(130,210,170,65,20,0,button,"GDelete");property(&b1,vga_font,NULL,BUTTONCOLOR);on_control_change(gdelete);
  p=samples_total!=0;
  set_enable(0,30,p);set_enable(0,40,p);set_enable(0,50,p);
  redraw_window();
  }

void main(int argc,char **argv)
  {

  if (argc!=2)
     {
     puts("MGF SoundTrack Editor ... (C) 1998 Ond�ej Nov�k\n");
     puts("Usage: MGIFEDIT <film.mgf>");
     exit(1);
     }
  initialization();
  set_mgif(argv[1]);
  prj_name=get_project_name(mgif_filename);
  switch (load_project(prj_name))
     {
     case -1:msg_box(PRG_HEADER,'\x1',"Chyba form�tu v projektu!","Zav��t",NULL);shutdown();exit(1);
     case -2:msg_box(PRG_HEADER,'\x1',"Neo�ek�van� eof souboru!","Zav��t",NULL);shutdown();exit(1);
     }
  init_editor();
  warn_size_mistmach();
  init_desktop();
  control_window();
  escape();
  shutdown();
  }
