#include <skeldal_win.h>
#include <libs/types.h>
#include <process.h>
#include <dos.h>
#include <mem.h>
#include <stdio.h>
#include <libs/bgraph.h>
#include <libs/event.h>
#include <libs/devices.h>
#include <libs/bmouse.h>
#include <libs/gui.h>

#include <libs/strlite.h>
#include <strlists.h>
#include "mapy.h"
#include <libs/basicobj.h>
#include <libs/memman.h>
#include "globals.h"
#include "edit_map.h"
#include "steny.h"
#include "save_map.h"
#include "wiz_tool.h"
#include "resource.h"

int temp_source;
int sektor_info=-1;
int sektor_win=-1;
int draw_win=-1;
int change_map;
char datapath[]="";
char last_directions=0;

char *strs(int num)
  {
  static char s[200];
  char **format;
  int *p;

  p=(int *)(o_end->userptr);
  format=(char **)p+4;
  sprintf(s,*format,num);
  return s;
  }

long vals(int num)
  {
  char s[200];
  long i;

  get_value(0,num,s);
  sscanf (s,"%d", &i);
  return i;
  }


void test_int(void)
  {
  int *p;
  char s[200];
  char **t;
  int i;

  p=(int *)(o_aktual->userptr);
  get_value(0,o_aktual->id,s);
  sscanf (s,"%d", &i);
  p=p+2;
  if (i<*p || i>*(p+1)) cancel_event();
  t=(char **)p+2;
  sprintf(s,*t,i);
  set_value(0,o_aktual->id,s);
  }

void fill_bit_fields(int win,int id_start,int source,int count)
  {
  while (count)
     {
     c_set_value(win,id_start,source & 1);
     id_start+=10;
     source>>=1;
     count--;
     }
  }

long get_bit_fields(int win,int id_start,int count)
  {
  long l;
  int mask;

  mask=0;l=0;
  while (count)
     {
     l|=f_get_value(win,id_start)<<mask;
     id_start+=10;
     mask++;
     count--;
     }
  return l;
  }


void string_list_sup_call()
  {
  int id,v;
  OBJREC *p;

  id=o_aktual->id;
  p=o_aktual;
  v=f_get_value(0,id);
  v=string_list(*(char **)(o_aktual->userptr),v);
  if (v+1) c_set_value(0,id,v);
  o_aktual=p;
  p->on_change();
  o_aktual=o_start;
   }

void string_list_sup()
  {
  run_background(string_list_sup_call);
  }

void str_line_init(OBJREC *o,void **x) //source_list
  {
  o->userptr=(void *)getmem(16);
  memcpy(o->userptr,x,sizeof(*x));
  }

void str_line_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  char c[200];
  char **s;
  int *d;


  d=(int *)o->data;
  bar(x1,y1,x2,y2);
  s=*(char ***)(o->userptr);
  if (*d<0 || *d>=str_count(s) || s[*d]==0) strcpy(c,"???");else strcpy(c,s[*d]);
  while (text_width(c)>(x2-x1))
     {
     char *t;

     t=strchr(c,'\0');
     *(--t)='\0';
     }
  position(x1,y1);
  outtext(c);
  }

void str_line_event(EVENT_MSG *msg,OBJREC *o)
  {
  if (msg->msg==E_CONTROL)
    {
    memcpy(o->userptr,msg->data,4);
    }
  }

void str_line(OBJREC *o)
  {
  o->call_init=str_line_init;
  o->call_draw=str_line_draw;
  o->call_event=str_line_event;
  o->datasize=4;
  }

static void refresh_vyk_map()
  {
  int i,j,s,d;

  for(i=1;i<maplen;i++)
     for(j=0;j<4;j++)
        mapa.sidedef[i][j].oblouk &= ~0x10;
  for(i=0;i<256;i++)
     {
     s=vyklenky[i].sector;
     d=vyklenky[i].dir;
     if (s!=0) mapa.sidedef[s][d].oblouk |= 0x10;
     }
  }


char edit_side_save(int target,int smer)
  {
  TSTENA nw;
  TSTENA *p=&nw;
  TSTENA *o;
  TSTENA chs;

   int apl,i, j;
   int selmode;

     apl=get_bit_fields(0,400,4);
     if (apl==0) return msg_box("Editor st�n",2,"Nen� za�krtnut� ��dn� strana. Pokra�ovat?","Ano","Ne",NULL)==1;
     if (smer!=-1)
     if (count_of_sel()>1)
      selmode=msg_box("Editor st�n",'\x2',"Je ozna�eno v�ce sektor�. Chce� upravit pouze ...","Aktu�ln�","V�echny","Zm�ny","Zp�t",NULL);
      else selmode=(apl!=1 && apl!=2 && apl!=4 && apl!=8 && target!=0)?3:1;
     else
      smer=0,apl=0xf,selmode=1;
     if (selmode==4) return 0;
     p->flags=get_bit_fields(0,120,20);
     p->flags|=f_get_value(0,350);
     p->prim_anim=vals(50)-1;
     p->sec_anim=vals(60)-1;
     p->xsec=vals(70)>>1;
     p->ysec=vals(80)>>1;
     p->lclip=vals(440);
     p->sector_tag=vals(90);
     p->side_tag=f_get_value(0,100);
     p->action=f_get_value(0,110);
     p->prim=f_get_value(0,20);
     p->sec=f_get_value(0,30);
     p->oblouk=f_get_value(0,40)+(f_get_value(0,450)<<5)+(f_get_value(0,470)<<7);
     p->side_tag|=(f_get_value(0,480)<<7);
     o=&mapa.sidedef[target][smer];
     if (selmode==3)
        {        
        memset(&chs,0,sizeof(chs));
		if (p->flags!=o->flags) chs.flags=o->flags ^ p->flags;
		if (p->prim_anim!=o->prim_anim) chs.prim_anim=0xFF;
		if (p->sec_anim!=o->sec_anim) chs.sec_anim=0xFF;
		if (p->xsec!=o->xsec) chs.xsec=0xFF;
		if (p->ysec!=o->ysec) chs.ysec=0xFF;
		if (p->lclip!=o->lclip) chs.lclip=0xFF;
		if (p->sector_tag!=o->sector_tag) chs.lclip=0xFF;
		if ((p->side_tag & 0x7F)!=(o->side_tag & 0x7F)) chs.side_tag|=0x7F;
		if ((p->side_tag & 0x80)!=(o->side_tag & 0x80)) chs.side_tag|=0x80;
		if (p->action!=o->action) chs.action=0xFF;
		if (p->prim!=o->prim) chs.prim=0xFF;
		if (p->sec!=o->sec) chs.sec=0xFF;
		if ((p->oblouk & 0x1F)!=(o->oblouk & 0x1F)) chs.sec|=0x1F;
		if ((p->oblouk & 0x60)!=(o->oblouk & 0x60)) chs.sec|=0x60;
		if ((p->oblouk & 0x80)!=(o->oblouk & 0x80)) chs.sec|=0x80;
        }
     else
        {
        memset(&chs,0xff,sizeof(chs));
        }
     for (i=0;i<4;(apl>>=1),i++) if (apl & 1)
        {
        o=&mapa.sidedef[target][i];
        move_changes(p,o,&chs,sizeof(TSTENA));
        if (selmode!=1)
           for(j=1;j<maplen;j++)
              if (minfo[j].flags & 0x1 && i!=target)
                 move_changes(p,&mapa.sidedef[j][i],&chs,sizeof(TSTENA));
        }
  refresh_vyk_map();
  return 1;
  }
void edit_side_ok()
  {
  int target,smer;

  target=f_get_value(0,360);
  smer=f_get_value(0,370);
  if (edit_side_save(target,smer))
     {
     close_current();
     info_sector(target);
     }
  }


void *edit_side_predvolba_0(EVENT_MSG *msg)
  {
  char *c;
  if (msg->msg==E_INIT) return &edit_side_predvolba_0;
  if (msg->msg==E_DONE) return NULL;
  c=(char *)msg->data;
  strcpy(c,"Vlastnosti nastaveny jako p�edvolba...");
  c=strchr(c,'\0');
  msg->data=(void *)c;
  msg->msg=-1;
  return NULL;
  }

void edit_side_predvolba_1(void)
  {
  edit_side_save(0,-1);
  send_message(E_STATUS_LINE,E_DONE,E_IDLE,edit_side_predvolba_0);
  send_message(E_STATUS_LINE,E_ADD,E_IDLE,edit_side_predvolba_0);
  }

void edit_side_predvolba_2(void)
  {
  send_message(E_STATUS_LINE,E_DONE,E_IDLE,edit_side_predvolba_0);
  }

void value_store_init(OBJREC *o,int *bytes)
  {
  o->datasize=*bytes;
  }

void value_store(OBJREC *o)
  {
  o->call_init=value_store_init;
  }

void action_flags()
  {
  CTL3D b1;
  long flags;

  flags=f_get_value(0,350);
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  def_dialoge(300,220,300,210,"V�ce vlajek");
  define(CANCEL_BUTT,11,5,80,20,2,button,"Zru�it");property(&b1,NULL,NULL,WINCOLOR);
  on_control_change(terminate_gui);
  define(OK_BUTT,100,5,80,20,2,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);
  on_control_change(terminate_gui);
  define(10,10,20,250,10,0,check_box,side_flgs[24]);
  define(20,10,32,250,10,0,check_box,side_flgs[25]);
  define(30,10,44,250,10,0,check_box,side_flgs[26]);
  define(40,10,56,250,10,0,check_box,side_flgs[27]);
  define(50,10,68,250,10,0,check_box,side_flgs[28]);
  define(60,10,88,250,10,0,check_box,side_flgs[29]);
  define(70,10,100,250,10,0,check_box,side_flgs[30]);
  define(80,10,112,250,10,0,check_box,side_flgs[31]);
  define(90,10,124,250,10,0,check_box,side_flgs[20]);
  define(100,10,136,250,10,0,check_box,side_flgs[21]);
  define(110,10,148,250,10,0,check_box,side_flgs[22]);
  define(120,10,160,250,10,0,check_box,side_flgs[23]);
  fill_bit_fields(0,10,flags>>24,8);
  fill_bit_fields(0,90,flags>>20,4);
  redraw_window();
  escape();
  if (o_aktual->id==OK_BUTT)
     {
     flags=get_bit_fields(0,10,8)<<24;
     flags|=get_bit_fields(0,90,4)<<20;

     }
  close_window(waktual);
  c_set_value(0,350,flags);
  }

int find_vyklenek_id(int sector,int dir)
  {
  int i;
  for(i=0;i<256;i++) if (vyklenky[i].sector==sector && vyklenky[i].dir==dir) return i;
  return -1;
  }

int find_free_vyklenek()
  {
  int i;
  for(i=0;i<256;i++) if (vyklenky[i].sector==0) return i;
  return -1;
  }


static void edit_vyklenek_start()
  {
  int smer,sector;
  int id;

  smer=f_get_value(0,370);
  sector=f_get_value(0,360);
  id=find_vyklenek_id(sector,smer);
  if (id==-1)
     {
     TVYKLENEK *v;
     id=find_free_vyklenek();
     if (id==-1)
        {
        msg_box("Omezeni!",'\x1',"V map� je pou�ito ji� mnoho v�klenk� ve zdi. Maxim�ln� po�et je 256","Ok",NULL);
        return;
        }
     v=vyklenky+id;
     v->xpos=250;
     v->ypos=160;
     v->xs=80;
     v->ys=60;
     v->sector=sector;
     v->dir=smer;
     v->items[0]=0;
     }
  edit_vyklenek(id);
  refresh_vyk_map();
  }

void edit_side(int source,int smer)
  {
  TSTENA *p;
  CTL3D b1,b2,b3;
  char s[200];

  p=&mapa.sidedef[source][smer];
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  sprintf(s,"Vlastnosti st�n, sektor %d st�na %d",source,smer);
  def_window(400,324,s);
  waktual->x=125;
  waktual->y=60;
  waktual->modal=1;
  define(CANCEL_BUTT,10,5,80,20,2,button,"Zru�it");property(&b1,NULL,NULL,WINCOLOR);
    on_control_change(close_current);
  define(OK_BUTT,190,5,80,20,2,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);
    on_control_change(edit_side_ok);
  define(3400,100,5,80,20,2,button,"P�edvolba");property(&b1,NULL,NULL,WINCOLOR);
    on_control_change(edit_side_predvolba_1);on_control_exit(edit_side_predvolba_2);
  define(10,5,20,300,35,0,label,"Prim�rn�:       Sekundarn�:       Oblouk:");property(&b3,NULL,NULL,WINCOLOR);
  define(20,10,35,90,12,0,str_line,side_names);property(&b2,NULL,NULL,WINCOLOR);c_default(p->prim);
    on_control_enter(string_list_sup);
  define(30,110,35,90,12,0,str_line,side_names);property(&b2,NULL,NULL,WINCOLOR);c_default(p->sec);
    on_control_enter(string_list_sup);
  define(40,210,35,90,12,0,str_line,oblouky);property(&b2,NULL,NULL,WINCOLOR);c_default(p->oblouk & 0x0f);
    on_control_enter(string_list_sup);
  define(-1,5,60,250,12,0,label,"Animace primarn�:");
  define(-1,5,75,250,12,0,label,"Animace sekundarn�:");
  define(-1,5,90,250,12,0,label,"X pozice sek. st�ny:");
  define(-1,5,105,250,12,0,label,"Y pozice sek. st�ny:");
  define(-1,5,120,250,12,0,label,"C�lov� sektor ud�losti:");
  define(-1,5,135,250,12,0,label,"C�lov� st�na ud�losti:");
  define(-1,5,150,250,12,0,label,"Popis akce:");
  define(-1,260,60,100,12,0,label,"Aplikuj na st�nu:");
  define(50,200,60,50,10,0,input_line,10,1,16,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs((p->prim_anim & 0xf)+1));on_control_exit(test_int);
  define(60,200,75,50,10,0,input_line,10,1,16,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs((p->sec_anim & 0xf)+1));on_control_exit(test_int);
  define(70,200,90,50,10,0,input_line,10,0,499,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs(p->xsec<<1));on_control_exit(test_int);
  define(80,200,105,50,10,0,input_line,10,0,511,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs(p->ysec<<1));on_control_exit(test_int);
  define(90,200,120,50,10,0,input_line,10,0,65535,"%6d");property(&b2,NULL,NULL,WINCOLOR);
   set_default(strs(p->sector_tag));on_control_exit(test_int);
  define(100,200,135,50,10,0,str_line,steny2);property(&b2,NULL,NULL,WINCOLOR);
   c_default(p->side_tag & 0x3);on_control_enter(string_list_sup);
  define(110,100,150,150,10,0,str_line,actions);property(&b2,NULL,NULL,WINCOLOR);
   on_control_enter(string_list_sup); c_default(p->action);
  define(120,10,175,150,10,0,check_box,side_flgs[0]);
  define(130,10,187,150,10,0,check_box,side_flgs[1]);
  define(140,10,199,150,10,0,check_box,side_flgs[2]);
  define(150,10,211,150,10,0,check_box,side_flgs[3]);
  define(160,10,223,150,10,0,check_box,side_flgs[4]);
  define(170,10,235,150,10,0,check_box,side_flgs[5]);
  define(180,10,247,150,10,0,check_box,side_flgs[6]);
  define(190,10,259,150,10,0,check_box,side_flgs[7]);
  define(200,190,175,100,10,0,check_box,side_flgs[8]);
  define(210,190,187,100,10,0,check_box,side_flgs[9]);
  define(220,190,199,100,10,0,check_box,side_flgs[10]);
  define(230,190,211,150,10,0,check_box,side_flgs[11]);
  define(240,190,223,150,10,0,check_box,side_flgs[12]);
  define(250,190,235,150,10,0,check_box,side_flgs[13]);
  define(260,190,247,150,10,0,check_box,side_flgs[14]);
  define(270,190,259,150,10,0,check_box,side_flgs[15]);
  define(280,10,271,75,10,0,check_box,side_flgs[16]);
  define(290,10,283,75,10,0,check_box,side_flgs[17]);
  define(300,190,271,150,10,0,check_box,side_flgs[18]);
  define(310,10,295,75,11,0,check_box,side_flgs[19]);
  define(400,290,80,100,10,0,check_box,"Severn�");
  define(410,290,95,100,10,0,check_box,"V�chodn�");
  define(420,290,110,100,10,0,check_box,"Ji�n�");
  define(430,290,125,100,10,0,check_box,"Z�padn�");
  define(340,290,150,100,15,0,button2,"V�ce >>");on_control_change(action_flags);
  define(350,290,150,100,15,0,value_store,4);c_default(p->flags & 0xfff00000);
  define(360,290,150,100,15,0,value_store,4);c_default(source);
  define(370,290,150,100,15,0,value_store,4);c_default(smer);
  define(-1,5,20,60,10,1,label,"Lclip:");
  define(440,30,35,30,12,1,input_line,3,0,255,"%3d");
   property(&b2,NULL,NULL,WINCOLOR);
  set_default(strs(p->lclip));on_control_exit(test_int);
  define(450,10,220,30,30,1,radio_butts,3,"-","\x4","\x6");c_default((p->oblouk>>5) & 0x3);
   property(NULL,icones,NULL,WINCOLOR);
  define(460,10,175,80,12,1,button,"V�klenek");on_control_change(edit_vyklenek_start);
  define(470,190,283,150,10,0,check_box,"Lze polo�it za");c_default(p->oblouk>>7);
  define(480,10,307,75,10,0,check_box,"P�edsunout s.");c_default(p->side_tag>>7);
  fill_bit_fields(0,120,p->flags,20); if (source)
  fill_bit_fields(0,400,1<<smer,4); else fill_bit_fields(0,400,0xf,4);
  set_enable(0,3400,source!=0);
  set_enable(0,460,source!=0);
  redraw_window();
  }

void edit_side_sup(void)
  {
  edit_side(temp_source,o_aktual->id/10-5);
  }

void edit_sector(int source)
  {
  TSECTOR *p;
  TSTR_LIST l;
  CTL3D b1,b2,b3;

  l=read_directory("c:\\windows\\system\\*.*",DIR_FULL,_A_NORMAL);
  p=&mapa.sectordef[source];
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(3,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,100,300,200,"String list - test only");
  define(9,10,20,240,170,0,listbox,l,0x1f);c_default(0);
  o_end->autoresizex=1;
  o_end->autoresizey=1;
  define(10,3,42,17,110,1,scroll_bar_v,0,10,1,SCROLLBARCOL);
  property(NULL,NULL,NULL,WINCOLOR);
  o_end->autoresizey=1;
  define(11,1,20,21,17,1,scroll_button,-1,0,"\x1e");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(12,1,22,21,17,2,scroll_button,1,10,"\x1f");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(20,1,1,10,10,2,resizer);
/*  define(OK_BUTT,100,5,80,20,2,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);
    on_change(terminate);
  define(CANCEL_BUTT,10,5,80,20,2,button,"Zru�it");property(&b1,NULL,NULL,WINCOLOR);
    on_change(terminate);
  define(-1,5,20,100,12,0,label,"P�ipojen�:");
  define(10,10,35,50,12,0,input_line,20,0,MAPSIZE-1,"%6d");property(&b2,NULL,NULL,WINCOLOR);
    set_default(strs(p->step_next[0]));on_exit(test_int);
  define(20,10,50,50,12,0,input_line,20,0,MAPSIZE-1,"%6d");property(&b2,NULL,NULL,WINCOLOR);
    set_default(strs(p->step_next[1]));on_exit(test_int);
  define(30,10,65,50,12,0,input_line,20,0,MAPSIZE-1,"%6d");property(&b2,NULL,NULL,WINCOLOR);
    set_default(strs(p->step_next[2]));on_exit(test_int);
  define(40,10,80,50,12,0,input_line,20,0,MAPSIZE-1,"%6d");property(&b2,NULL,NULL,WINCOLOR);
    set_default(strs(p->step_next[3]));on_exit(test_int);
  define(50,70,35,80,12,0,button,"Sever");property(&b1,NULL,NULL,WINCOLOR);on_change(edit_side_sup);
  define(60,70,50,80,12,0,button,"V�chod");property(&b1,NULL,NULL,WINCOLOR);on_change(edit_side_sup);
  define(70,70,65,80,12,0,button,"Jih");property(&b1,NULL,NULL,WINCOLOR);on_change(edit_side_sup);
  define(80,70,80,80,12,0,button,"Z�pad");property(&b1,NULL,NULL,WINCOLOR);on_change(edit_side_sup);
  temp_source=source;
  */redraw_window();
  escape();
  close_window(waktual);
  release_list(l);
  }

void run_edit_side(void)
  {
  char s[100];
  int i;
  OBJREC *o;

  get_value(0,5,s);
  if (strcmp(s,"?"))
     {
     sscanf(s,"%d",&i);
     o=o_aktual;
     edit_side(i,o_aktual->id/10-10);
     }

  }


void Apply(void)
  {
  int sec,i;
  char s[100];

  get_value(0,5,s);
  if (!strcmp(s,"?")) return;
  sscanf(s,"%d",&sec);
     {
     TSECTOR *p;int j;

     p=&mapa.sectordef[sec];
     for (j=0;j<4;j++) p->step_next[j]=(word)vals((j+1)*10);
     }
  for (i=1;i<maplen;i++)
     if (minfo[i].flags & 1)
        {
        int j;

        for (j=0;j<4;j++)
           if (change_map & (1<<j))
             mapa.sidedef[i][j].prim=(char)f_get_value(0,(j+5)*10);
        if(change_map & 0x10) mapa.sectordef[i].ceil=(char)f_get_value(0,200);
        if(change_map & 0x20) mapa.sectordef[i].floor=(char)f_get_value(0,210);
        if(change_map & 0x40) mapa.sectordef[i].sector_type=(char)f_get_value(0,220);
     }
  redraw_desktop();
  }


void jdi_na_sektor(s)
  {
  WINDOW *w,*w2;
  if (!s) return;
  cur_layer=minfo[s].layer;
  xmap_offs=minfo[s].x;
  ymap_offs=minfo[s].y;
  c_set_value(map_win,20,ymap_offs);
  c_set_value(map_win,30,xmap_offs);
  unselect_map();
  minfo[s].flags=1;
  info_sector(s);
  w=find_window(map_win);
  w2=waktual;
  if (w!=NULL)
     {
     waktual=w;
     redraw_window();
     waktual=w2;
     }
  }

void chozeni(EVENT_MSG *msg)
  {
  MS_EVENT *ms;

  if (msg->msg==E_MOUSE)
     {
     ms=get_mouse(msg);
     if (ms->event_type & 0x8)
        {
        int s;
        s=vals(o_aktual->id);
        jdi_na_sektor(s);
        }
     }
  }

void chozeni2(EVENT_MSG *msg,OBJREC *o)
  {
  word s2,s1;
  char ok=1;

  if (msg->msg==E_INIT) return ;
  if (msg->msg==E_DONE) return ;
  if (msg->msg==E_KEYBOARD)
     {
     if (o_aktual==NULL || o_aktual->on_change!=chozeni2) return;
     if (waktual->id==map_win || waktual->id==tool_bar)
        select_window(sektor_win);
     if (waktual->id!=sektor_win) return;
     s2=vals(5);s1=s2;
     if (s2>maplen) return;
     if (!(*(char *)msg->data))
     switch (*(int *)msg->data>>8)
        {
        case 'H':s1=mapa.sectordef[s2].step_next[0];break;
        case 'P':s1=mapa.sectordef[s2].step_next[2];break;
        case 'M':s1=mapa.sectordef[s2].step_next[1];break;
        case 'K':s1=mapa.sectordef[s2].step_next[3];break;
        default:ok=0;
        }
     else if (*(char *)msg->data!=13) ok=0;
     if (ok)
        {
        if (s1>maplen) return;
        jdi_na_sektor(s1);
        while (_bios_keybrd(_KEYBRD_READY)) _bios_keybrd(_KEYBRD_READ);
        msg->msg=-1;
        }
     }
  if (msg->msg==E_GET_FOCUS)
     {
     memcpy(&o->f_color,flat_color(0x3e0),sizeof(FC_TABLE));
     redraw_object(o);
     }
  if (msg->msg==E_LOST_FOCUS)
     {
     memcpy(&o->f_color,flat_color(0),sizeof(FC_TABLE));
     redraw_object(o);
     }

  return ;
  }

void sector_details_call(int sect_num)
  {
  TSECTOR *p;
  CTL3D b1,b2,b3;
  int start,end,i;
  char s[200];

  p=&mapa.sectordef[sect_num];
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(3,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  sprintf(s,"Detaily sektoru %d",sect_num);
  def_dialoge(100,50,440,340,s);
  define(-1,30,30,100,12,0,label,"Strop:");
  define(-1,30,50,100,12,0,label,"Podlaha:");
  define(-1,30,70,100,12,0,label,"Sektor:");
  define(-1,30,90,100,12,0,label,"C�l akce:");
  define(-1,30,110,100,12,0,label,"St�na akce:");
  define(-1,30,130,100,12,0,label,"Popis akce:");
  define(10,30,28,100,13,1,str_line,ceils);property(&b2,NULL,NULL,WINCOLOR);
     c_default(p->ceil); on_control_enter(string_list_sup);
  define(20,30,48,100,13,1,str_line,floors);property(&b2,NULL,NULL,WINCOLOR);
     c_default(p->floor); on_control_enter(string_list_sup);
  define(30,30,68,100,13,1,str_line,sector_types);property(&b2,NULL,NULL,WINCOLOR);
     c_default(p->sector_type); on_control_enter(string_list_sup);
  define(40,30,88,50,13,1,input_line,10,0,MAPSIZE,"%6d");property(&b2,NULL,NULL,WINCOLOR);
     set_default(strs(p->sector_tag));on_control_exit(test_int);
  define(50,30,108,100,13,1,str_line,steny2);property(&b2,NULL,NULL,WINCOLOR);
     c_default(p->side_tag & 3); on_control_enter(string_list_sup);
  define(60,30,128,150,13,1,str_line,actions);property(&b2,NULL,NULL,WINCOLOR);
     c_default(p->action); on_control_enter(string_list_sup);
  define(70,5,150,214,80,0,radio_butts,7,
       "(1) Norm�ln� podlaha",
       "(2) Dva druhy podlah",
       "(3) Dva sm�ry",
       "(4) Dva sm�ry a druhy",
       "(5) �ty�i sm�ry",
       "(6) �ty�i sm�ry a dva druhy",
       "(7) �achovnice");c_default(p->flags & 0x7);
  define(80,5,150,214,80,1,radio_butts,7,
       "(1) Norm�ln� strop",
       "(2) Dva druhy strop",
       "(3) Dva sm�ry",
       "(4) Dva sm�ry a druhy",
       "(5) �ty�i sm�ry",
       "(6) �ty�i sm�ry a dva druhy",
       "(7) �achovnice");c_default(p->flags >> 4 & 0x7);
  define(90,5,240,214,10,0,check_box,"(?)Animace podlahy");c_default((p->flags & 0x8)!=0);
  define(95,5,240,214,10,1,check_box,"(?)Animace stropu");c_default((p->flags & 0x80)!=0);
  define(65,5,252,214,10,0,check_box,"Sekundarni shading");c_default((minfo[sect_num].flags & 0x100)>>8);
  define(75,5,258,214,40,1,radio_butts,4,"*default*","�ipka","Schody","Bez symbolu");c_default((minfo[sect_num].flags & 0x600)>>9);
  define(83,5,264,214,10,0,check_box,"!Automap (kouzlem)");c_default((minfo[sect_num].flags & 0x800)>>11);
  define(85,5,276,214,10,0,check_box,"!Summon");c_default((minfo[sect_num].flags & 0x1000)>>12);
  define(87,5,288,214,10,0,check_box,"Neprojde hledan�m cesty");c_default((minfo[sect_num].flags & 0x2000)>>13);
  define(100,10,10,80,20,3,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(110,10,10,80,20,2,button,"Zru�it");property(&b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(120,100,10,80,20,2,button,"P�edvolba");property(&b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  set_enable(0,120,sect_num!=0);
  redraw_window();
  do
     {
     sem:
     escape();
     if (o_aktual->id-110)
        {
          i=0;
        if (o_aktual->id==120)
           {
           start=0;end=0;minfo[0].flags|=1;
           }
        else
           {
           if (count_of_sel()<2 || (i=msg_box("Co te�?",'\x2',"M�m vlastnosti aplikovat na jeden sektor nebo na vybranou oblast?",
                                "Na sektor","Na oblast","Zru�it",NULL))==1)
                                {
                                start=sect_num;
                                end=sect_num;
                                minfo[sect_num].flags |=1;
                                }
                             else
                                {
                                start=1;
                                end=maplen-1;
                                }
           }
        if (i==3) goto sem;
        else
        for (i=start;i<=end;i++) if (minfo[i].flags & 1)
           {
           p=&mapa.sectordef[i];
           p->ceil=f_get_value(0,10);
           p->floor=f_get_value(0,20);
           p->sector_type=f_get_value(0,30);
           p->sector_tag=vals(40);
           p->side_tag=f_get_value(0,50);
           p->action=f_get_value(0,60);
           p->flags=f_get_value(0,70)+16*f_get_value(0,80)+8*f_get_value(0,90)+0x80*f_get_value(0,95);;
           minfo[i].flags&=~0x1f00;
           minfo[i].flags|=(f_get_value(0,65)<<8)|(f_get_value(0,75)<<9)|(f_get_value(0,83)<<11)|(f_get_value(0,85)<<12)|(f_get_value(0,87)<<13);
           }
        }
     }
  while (o_aktual && o_aktual->id==120);
  close_window(waktual);
  minfo[0].flags &= ~1;
  }

void set_change_map(void)
  {
  int i;

  i=o_aktual->id / 10-5;
  if (i>14) i-=11;
  change_map|=1<<i;
  }

void sector_details(void)
  {
  int i;

  i=vals(5);
  sector_details_call(i);
  info_sector(i);
  }

void get_error(char *err)
  {
/*  char *c,mezera,spc;
  int i;
  short *p;

  p=(short *)0xb8000;
  if (p[0]==0xff)
     {
     strcpy(err,"Neo�ek�van� zhroucen�.");
     return;
     }
  mezera=1;spc=1;
  c=err;
  for(i=0;i<160;i++)
     {
     *c=*p++ & 0xff;
     if (*c<33)
        {
        c+=mezera;mezera=0;
        if (spc) spc++;
        }
     else
        {
        c++;mezera=1;
        spc=0;
        }
     if (spc>10) break;
     }
  *c=0;
  if (c!=err)
     {
     do
        c--;
     while (c!=err && *c==32);
     if (*c!=32) c++;
     *c=0;
     }*/
  }

void call_testmap(int pos)
  {
  char tt[250];
  HWND hWnd;

  if (save_all_map()) return;
  save_items();

  LoadString(GetModuleHandle(NULL),3,tt,250);
  hWnd=FindWindow(NULL,tt);
  if (hWnd==NULL)
	{
	msg_box("Testovani mapy",'\x1',"Pro testovani mapy, spust hru Skeldal.exe v okne s tvym dobrodruzstvim "
	  "Vytvor novou skupinu, popripade nahraj pozici."
	  "Po nahrati urovne muzes zacit hru testovat. MapEdit se propoji s hrou.","Ok",NULL);
	}
  else
	{
	const char *fname=strrchr(filename,'\\');
	ATOM atm;
	LRESULT res;

	if (fname==NULL) fname=filename;else fname++;
	atm=GlobalAddAtom(fname);	
	SendMessageTimeout(hWnd,WM_RELOADMAP,pos,(LPARAM)atm,SMTO_NORMAL,2000,&res);
	GlobalDeleteAtom(atm);
	BringWindowToTop(hWnd);
	}  
  }

void testmap(void)
  {
  call_testmap(vals(5));
  }

void close_sector_win(void)
  {
  close_window(waktual);
  send_message(E_DONE,E_KEYBOARD,chozeni2);
  }

void open_sector_win(void)
  {
  if (find_window(sektor_win)==NULL)
     {
     CTL3D b1,b2;
     FC_TABLE f_sel;

     memcpy(&b1,def_border(1,0),sizeof(CTL3D));
     memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
     default_font=vga_font;
     memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
     memcpy(&f_sel,flat_color(0x0017),sizeof(charcolors));
     sektor_win=def_window(120,325,"Vlastnosti");
     waktual->y=2;waktual->x=SCR_WIDTH_X-120-3;
     on_control_change(close_sector_win);
     define(100,4,19,59,14,0,button2,"Sever:");on_control_change(run_edit_side);
     define(110,4,49,59,14,0,button2,"V�chod:");on_control_change(run_edit_side);
     define(120,4,79,59,14,0,button2,"Jih:");on_control_change(run_edit_side);
     define(130,4,109,59,14,0,button2,"Z�pad:");on_control_change(run_edit_side);
     define(-1,5,140,59,10,0,label,"Strop:");
     define(-1,5,170,59,10,0,label,"Podlaha:");
     define(-1,5,200,59,10,0,label,"Typ sektoru:");
     define(10,65,20,50,12,0,input_line,20,0,MAPSIZE-1,"%5d");property(&b2,NULL,&f_sel,WINCOLOR);
     set_default("0");on_control_exit(test_int);on_event(chozeni);
     define(20,65,50,50,12,0,input_line,20,0,MAPSIZE-1,"%5d");property(&b2,NULL,&f_sel,WINCOLOR);
     set_default("0");on_control_exit(test_int);on_event(chozeni);
     define(30,65,80,50,12,0,input_line,20,0,MAPSIZE-1,"%5d");property(&b2,NULL,&f_sel,WINCOLOR);
     set_default("0");on_control_exit(test_int);on_event(chozeni);
     define(40,65,110,50,12,0,input_line,20,0,MAPSIZE-1,"%5d");property(&b2,NULL,&f_sel,WINCOLOR);
     set_default("0");on_control_exit(test_int);on_event(chozeni);
     define(50,5,35,95,12,0,str_line,side_names);property(&b2,NULL,&f_sel,WINCOLOR);
     c_default(0); on_control_enter(string_list_sup);on_control_change(set_change_map);
     define(60,5,65,95,12,0,str_line,side_names);property(&b2,NULL,&f_sel,WINCOLOR);
     c_default(0); on_control_enter(string_list_sup);on_control_change(set_change_map);
     define(70,5,95,95,12,0,str_line,side_names);property(&b2,NULL,&f_sel,WINCOLOR);
     c_default(0); on_control_enter(string_list_sup);on_control_change(set_change_map);
     define(80,5,125,95,12,0,str_line,side_names);property(&b2,NULL,&f_sel,WINCOLOR);
     c_default(0); on_control_enter(string_list_sup);on_control_change(set_change_map);
     define(140,103,35,12,12,0,check_box,"");o_end->call_event=o_end->on_change;
     define(150,103,65,12,12,0,check_box,"");o_end->call_event=o_end->on_change;
     define(160,103,95,12,12,0,check_box,"");o_end->call_event=o_end->on_change;
     define(170,103,125,12,12,0,check_box,"");o_end->call_event=o_end->on_change;
     define(200,5,155,110,12,0,str_line,ceils);property(&b2,NULL,&f_sel,WINCOLOR);
     c_default(0); on_control_enter(string_list_sup);on_control_change(set_change_map);
     define(210,5,185,110,12,0,str_line,floors);property(&b2,NULL,&f_sel,WINCOLOR);
     c_default(0); on_control_enter(string_list_sup);on_control_change(set_change_map);
     define(220,5,215,110,12,0,str_line,sector_types);property(&b2,NULL,&f_sel,WINCOLOR);
     c_default(0); on_control_enter(string_list_sup);on_control_change(set_change_map);
     define(-1,1,30,60,10,3,label,"Sektor:");
     define(OK_BUTT,60,67,55,17,3,button2,"Aplikuj");on_control_change(Apply);
     define(300,4,67,55,17,3,button2,"Detaily");on_control_change(sector_details);
     define(5,60,30,55,12,3,input_line,20,0,MAPSIZE-1,"%5d");property(&b2,NULL,NULL,WINCOLOR);
     set_default("?");on_control_change(chozeni2);
     define(310,4,3,112,20,3,button,"Testovat mapu");on_control_change(testmap);
     define(320,4,47,55,17,2,button2,"Zoom+");on_control_change(zoomin);
     define(330,4,47,55,17,3,button2,"Zoom-");on_control_change(zoomout);
     send_message(E_ADD,E_KEYBOARD,chozeni2);
     waktual->popup=1;
     }
  else
     {
     select_window(sektor_win);
     }
  redraw_window();
  }


void info_sector(int sector)
  {
  char c[100];
  WINDOW *w;
  int i;

  if (!sector) return;
  w=waktual;
  if (waktual->id!=sektor_win) open_sector_win();
  sprintf(c,"%5d",sector);
  set_value(0,5,c);
  for(i=0;i<4;i++)
     {
     sprintf(c,"%5d",mapa.sectordef[sector].step_next[i]);
     set_value(0,(i+1)*10,c);
     }
  c_set_value(0,50,mapa.sidedef[sector][0].prim);
  c_set_value(0,60,mapa.sidedef[sector][1].prim);
  c_set_value(0,70,mapa.sidedef[sector][2].prim);
  c_set_value(0,80,mapa.sidedef[sector][3].prim);
  c_set_value(0,140,(mapa.sidedef[sector][0].flags & 0x200)!=0);
  c_set_value(0,150,(mapa.sidedef[sector][1].flags & 0x200)!=0);
  c_set_value(0,160,(mapa.sidedef[sector][2].flags & 0x200)!=0);
  c_set_value(0,170,(mapa.sidedef[sector][3].flags & 0x200)!=0);
  c_set_value(0,200,mapa.sectordef[sector].ceil);
  c_set_value(0,210,mapa.sectordef[sector].floor);
  c_set_value(0,220,mapa.sectordef[sector].sector_type);
  select_window(w->id);
  change_map=0;
  }

void draw_win_1(void)
  {
  int i;

  for (i=20;i<70;i++) c_set_value(draw_win,i,0);
  c_set_value(draw_win,10,1);
  }

void draw_win_2(void)
  {
  c_set_value(draw_win,10,get_bit_fields(draw_win,10,6)==0);;
  }


void open_draw_win(void)
  {
  if (find_window(draw_win)==NULL)
     {
     FC_TABLE f_sel;

     default_font=vga_font;
     memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
     memcpy(&f_sel,flat_color(0x0017),sizeof(charcolors));
     draw_win=def_window(120,165,"Vlajky");
     waktual->y=2;waktual->x=SCR_WIDTH_X-120-3;
     define(10,5,30,100,10,0,check_box,"Rozd�lit");c_default(1);
      on_control_change(draw_win_1);
     define(20,5,45,100,10,0,check_box,"!Hra�");c_default(0);
      on_control_change(draw_win_2);
     define(30,5,60,100,10,0,check_box,"!Nestv�ra");c_default(0);
      on_control_change(draw_win_2);
     define(40,5,75,100,10,0,check_box,"!V�c");c_default(0);
      on_control_change(draw_win_2);
     define(50,5,90,100,10,0,check_box,"!Zvuk");c_default(0);
      on_control_change(draw_win_2);
     define(60,5,105,100,10,0,check_box,"PrimVis");c_default(0);
      on_control_change(draw_win_2);
     define(70,5,18,110,17,3,button2,"Zoom in");on_control_change(zoomin);
     define(80,5,1,110,17,3,button2,"Zoom out");on_control_change(zoomout);
     waktual->popup=1;

     }
  else
     {
     select_window(sektor_win);
     }
  redraw_window();
  }

long get_draw_flags(void)
  {
  long l;
  if (find_window(draw_win)!=NULL)
     {
     l=get_bit_fields(draw_win,10,6);
     if (l & 1)l=0; else
     if (l & 0x20) l=(l & 0x1f)|SD_PRIM_VIS;
     }
  else
     l=0xff91;
  return l;
  }


static void veci_ve_vyklenku(TSTR_LIST *ls,short *list)
  {
  int i;
  if (*ls!=NULL) release_list(*ls);
  *ls=create_list(8);
  for(i=0;list[i]!=0;i++)
     {
     int it=list[i]-1;
     if (it>=max_items) str_add(ls,"<P�edm�t neexistuje>");else str_add(ls,item_list[it].jmeno);
     }
  }

static void add_to_vyk()
  {
  int i,p;
  TSTR_LIST ls;
  TSTR_LIST ls2;
  TVYKLENEK *v;
  int id;

  i=f_get_value(0,9);
  p=f_get_value(0,29);
  id=f_get_value(0,20);
  send_message(E_GUI,29,E_CONTROL,0,&ls);
  send_message(E_GUI,9,E_CONTROL,0,&ls2);
  if (ls2[i]==NULL) return;
  sscanf(ls2[i],"%d",&i);
  if (i==-1) return;
  v=vyklenky+id;
  if (p!=7)memmove(v->items+p+1,v->items+p,(7-p)*2);
  v->items[8]=0;
  v->items[p]=i+1;
  veci_ve_vyklenku(&ls,&v->items);
  send_message(E_GUI,29,E_CONTROL,1,ls);
  }

static void remove_from_vyk()
  {
  int i;
  TVYKLENEK *v;
  TSTR_LIST ls;
  int id;

  i=f_get_value(0,29);
  if (i>=8) return;
  id=f_get_value(0,20);
  v=vyklenky+id;
  memmove(&v->items[i],&v->items[i+1],(8-i)*2);
  send_message(E_GUI,29,E_CONTROL,0,&ls);
  veci_ve_vyklenku(&ls,&v->items);
  send_message(E_GUI,29,E_CONTROL,1,ls);
  }

static change_grep()
  {
  TSTR_LIST ls;
  char grep_en;
  int grep_type;
  int grep_num;

  c_set_value(0,9,0);
  send_message(E_GUI,9,E_CONTROL,2);
  grep_en=f_get_value(0,90);
  grep_type=f_get_value(0,80);
  if (grep_en) grep_num=grep_type;else grep_num=-1;
  send_message(E_GUI,9,E_CONTROL,0,&ls);
  create_isort_list(&ls,grep_num);
  send_message(E_GUI,9,E_CONTROL,1,ls);
  redraw_desktop();
  }

void edit_vyklenek(int idnum)
  {
  TVYKLENEK *v=vyklenky+idnum;
  TSTR_LIST vyklist=NULL;
  TSTR_LIST ls_sorts=NULL;
  CTL3D b1,b2,b3;
  TVYKLENEK old;

  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  memcpy(&old,v,sizeof(TVYKLENEK));
  veci_ve_vyklenku(&vyklist,&v->items);
  create_isort_list(&ls_sorts,-1);
  def_dialoge(70,100,500,200,"Oprava v�klenk�");
  define(9,10,20,200,126,0,listbox,ls_sorts,RGB555(31,31,31),0);
  property(&b3,NULL,NULL,WINCOLOR);c_default(0);
  define(10,216,40,21,87,0,scroll_bar_v,0,10,1,SCROLLBARCOL);
  property(&b2,NULL,NULL,WINCOLOR);
  define(11,216,20,21,17,0,scroll_button,-1,0,"\x1e");
  property(&b1,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(12,216,130,21,17,0,scroll_button,1,10,"\x1f");
  property(&b1,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(20,0,0,0,0,0,value_store,4);c_default(idnum);
  define(29,10,20,200,120,1,listbox,vyklist,RGB555(31,31,31),0);
  property(&b3,NULL,NULL,WINCOLOR);c_default(0);
  define(40,242,40,40,15,0,button,">>");on_control_change(add_to_vyk);
  define(50,242,60,40,15,0,button,"<<");on_control_change(remove_from_vyk);
  define(80,30,150,120,12,1,str_line,typy_veci);c_default(0);
  property(&b2,NULL,NULL,WINCOLOR);on_control_enter(string_list_sup);on_control_change(change_grep);
  define(-1,150,150,50,10,1,label,"Filtr:");
  define(90,10,150,10,10,1,check_box,"");c_default(0);on_control_change(change_grep);
  define(-1,10,30,1,10,3,label,"Xpos  Ypos  Xsiz  Ysiz  Sector  Pos");
  define(100,8,12,40,12,3,input_line,10,0,500,"%4d");set_default(strs(v->xpos));
  property(&b2,NULL,NULL,WINCOLOR);on_control_exit(test_int);
  define(110,53,12,40,12,3,input_line,10,0,500,"%4d");set_default(strs(v->ypos));
  property(&b2,NULL,NULL,WINCOLOR);on_control_exit(test_int);
  define(120,98,12,40,12,3,input_line,10,0,500,"%4d");set_default(strs(v->xs));
  property(&b2,NULL,NULL,WINCOLOR);on_control_exit(test_int);
  define(130,143,12,40,12,3,input_line,10,0,500,"%4d");set_default(strs(v->ys));
  property(&b2,NULL,NULL,WINCOLOR);on_control_exit(test_int);
  define(140,188,12,40,12,3,input_line,10,0,maplen,"%4d");set_default(strs(v->sector));
  property(&b2,NULL,NULL,WINCOLOR);on_control_exit(test_int);
  define(150,233,12,40,12,3,input_line,10,0,3,"%4d");set_default(strs(v->dir));
  property(&b2,NULL,NULL,WINCOLOR);on_control_exit(test_int);
  define(200,5,5,60,20,2,button,"Ok");on_control_change(terminate_gui);property(&b1,NULL,NULL,WINCOLOR);
  define(210,70,5,60,20,2,button,"Zru�it");on_control_change(terminate_gui);property(&b1,NULL,NULL,WINCOLOR);
  define(220,135,5,60,20,2,button,"Vymazat");on_control_change(terminate_gui);property(&b1,NULL,NULL,WINCOLOR);
  redraw_window();
  opp:
  escape();
  if (o_aktual->id==200)
     {
     v->xpos=vals(100);
     v->ypos=vals(110);
     v->xs=vals(120);
     v->ys=vals(130);
     v->sector=vals(140);
     v->dir=vals(150);
     }
  else if (o_aktual->id==220)
     if (msg_box("Mapedit",'\x2',"Chce� opravdu v�klenek zbourat?","Ano","Ne",NULL)==1)
     {
     v->sector=0;
     }
     else goto opp;
  else if (o_aktual->id==210)
     {
     memcpy(v,&old,sizeof(old));
     }
  send_message(E_GUI,9,E_CONTROL,0,&ls_sorts);
  release_list(vyklist);
  release_list(ls_sorts);
  close_current();
  }
