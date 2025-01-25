#include <skeldal_win.h>
#include <types.h>
#include <mem.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <bgraph.h>
#include <event.h>
#include <devices.h>
#include <bmouse.h>
#include <gui.h>
#include <strlite.h>
#include <strlists.h>
#include <inicfg.h>
#include <basicobj.h>
#include <memman.h>
#include <pcx.h>
#include <dos.h>
#include "mapy.h"
#include "globals.h"
#include "edit_map.h"
#include "steny.h"
#include "save_map.h"
#include "mob_edit.h"

int pcxview_win=-1;
static char swap=1;
static char in_ddl=0;

static void *last_pcx=NULL;
static char *basic_path;

#define BASIC_PATH "..\\graphics\\"


TSTR_LIST read_ddl_dir(char *filter)
  {
  TSTR_LIST ls;
  int i=0;
  char *s;

  ls=create_list(256);
  s=read_next_entry(MMR_FIRST);
  while (s!=NULL)
     {
     char *d;
     char c[13];

     strncpy(c,s,12);
     c[12]=0;
     if (filter!=NULL)
        {
        d=strrchr(c,'.');
        if (d!=NULL && !strncmp(d+1,filter,3)) str_replace(&ls,i++,c);
        }
     else
        str_replace(&ls,i++,c);
     s=read_next_entry(MMR_NEXT);
     }
  if (i==0) str_add(&ls,"<chyba>");
  ls=sort_list(ls,-1);
  return ls;
  }

static void get_directory()
  {
  TSTR_LIST ls;
  char pathname[256];
  char file=0;

  c_set_value(0,9,0);
  send_message(E_GUI,9,E_CONTROL,2);
  get_value(0,20,pathname);
  file=f_get_value(0,45);
  if (pathname[0]==0) strcpy(pathname,".\\");
  else
  if (pathname[strlen(pathname)-1]!='\\') strcat(pathname,"\\");
  set_value(0,20,pathname);
  send_message(E_GUI,9,E_CONTROL,0,&ls);
  if (ls!=NULL) release_list(ls);
  if (file)
     ls=read_ddl_dir("PCX");
  else
     {
     strcat(pathname,"*.pcx");
     ls=read_directory(pathname,DIR_BREIF,_A_NORMAL);
     }
  if (ls==NULL)
     {
     ls=create_list(2);
     str_add(&ls,"<nic>");
     }
  send_message(E_GUI,9,E_CONTROL,1,ls);
  in_ddl=file;
  redraw_desktop();
  }

static void close_pcx_window()
  {
  TSTR_LIST ls;
  send_message(E_GUI,9,E_CONTROL,0,&ls);
  if (ls!=NULL) release_list(ls);
  if (last_pcx!=NULL)free(last_pcx);last_pcx=NULL;
  close_current();
  }

static void picture2_display(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  word *paleta;
  word *data;
  int xs,ys,xo;
  word *w;
  char *d,*e;

  data=*(void **)o->data;x2,y2;
  if (data==NULL) return;
  paleta=data+3;
  xs=data[0];
  ys=data[1];
  if (swap) y1=y1+ys/2;
  data=paleta+256;
  e=(char *)data;
  do
    {
    w=GetScreenAdr()+x1+scr_linelen2*y1;
    xo=xs/2;
    d=e;
    do
        {
		word col=paleta[*d];
        *w++=col/*+(col & ~0x1F)*/;d+=2;
        }
    while (--xo);
    e+=xs*2;
    if (swap) y1--;else y1++;
    ys-=2;
    }
  while(ys>0 && y1<460);
  }

static void picture2(OBJREC *o)
  {
  o->call_draw=picture2_display;
  o->datasize=4;
  }

static char *get_pcx(char *filename)
  {
  char *c;
  if (in_ddl)
     {
     long s;
     void *z;
     char *name;

     name=strrchr(filename,'\\');if(name!=NULL) name++;else name=filename;
     if (test_file_exist(read_group(0),name))
          {
          z=afile(name,read_group(0),&s);
          load_pcx(z,s,A_8BIT,&c);
          free(z);
          }
     else
        c=NULL;
     }
  else
     if (open_pcx(filename,A_8BIT,&c))
        {
        in_ddl=!in_ddl;c=get_pcx(filename);in_ddl=!in_ddl;
        return c;
        }
  return c;
  }

static void read_pcx()
  {
  char pathname[256];
  char name[30];
  TSTR_LIST ls;
  int i;

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  get_value(0,20,pathname);
  if (last_pcx!=NULL) free(last_pcx);
  i=f_get_value(0,9);
  strcpy(name,ls[i]);name_conv(name);
  strcat(pathname,name);
  last_pcx=get_pcx(pathname);
  c_set_value(0,30,(int)last_pcx);
  redraw_window();
  }

static void change_swap()
  {
  swap=f_get_value(0,40);
  redraw_window();
  }

static char clr_sw=1;

static void refresh_scrs()
  {
  WINDOW *w;
  int i;
  set_value(0,100,"");
  set_value(0,110," 1");
  switch(i=f_get_value(0,130))
    {
    case 0:send_message(E_GUI,140,E_CONTROL,side_names);break;
    case 1:send_message(E_GUI,140,E_CONTROL,floors);break;
    case 2:send_message(E_GUI,140,E_CONTROL,ceils);break;
    case 3:send_message(E_GUI,140,E_CONTROL,oblouky);break;
    }
  if (o_aktual && o_aktual->id==130)
    {
    if (i==3) fill_bit_fields(0,160,2+4,3);else fill_bit_fields(0,160,1,3);
    set_enable(0,160,i!=3);
    set_enable(0,170,i!=2 && i!=1);
    set_enable(0,180,i!=2 && i!=1);
    }
  c_set_value(0,140,0);
  set_value(0,150," 1");
  Beep(1000,10);
  w=find_window(sektor_win);
  if (w!=NULL) close_window(w);
  clr_sw=1;
  }

static void clear_switchs()
  {
  if (clr_sw)
    {
    c_set_value(0,160,0);
    c_set_value(0,170,0);
    c_set_value(0,180,0);
    c_set_value(0,o_aktual->id,1);
    clr_sw=0;
    }
  }


#define ZMENIT "Zm�nit grafiku"
static void zmenit()
  {
  int i,c,f;
  char *name;
  TSTR_LIST ls;

  i=f_get_value(0,9);
  send_message(E_GUI,9,E_CONTROL,0,&ls);
  if (i>=str_count(ls))return;
  if (ls[i]==NULL) return;
  name=alloca(strlen(ls[i])+1);strcpy(name,ls[i]);
  name_conv(name);
  c=f_get_value(0,140);
  if (c==0)
    {
    msg_box(ZMENIT,1,"Mus�� vybrat n�jakou grafiku. Klepni tam, jak je naps�no <nic>",MSB_OK);
    return;
    }
  load_side_script(script_name);
  f=vals(150);
  i=change_side(f_get_value(0,130),c-1,get_bit_fields(0,160,3),f-1,name);
  if (i<-1)
    {
    msg_box(ZMENIT,1,"Program odm�tl (z n�jak�ch d�vod�) akci prov�st. Zkontrolujte v�echny �daje",MSB_OK);
    discharge_side_script();
    return;
    }
  if (i==-1)
    {
    msg_box(ZMENIT,1,"Nepovolen� kombinace pol� \"Hlavn�\", \"Lev�\" a \"Prav�\"",MSB_OK);
    discharge_side_script();
    return;
    }
  save_side_script(script_name);
  refresh_scrs();
  c_set_value(0,140,c);
  if (i>0)
    {
    char c[20];

    set_value(0,150,itoa(f+1,c,10));
    }
  }

#define VYMAZAT "Vymazat"
static void vymazat()
  {
  int pos=f_get_value(0,140);

  if (pos==0)
    {
    msg_box(VYMAZAT,1,"Mus�� vybrat n�jakou grafiku. Klepni tam, jak je naps�no <nic>",MSB_OK);
    return;
    }
  if (msg_box(VYMAZAT,1,"Vymazat grafiku v�etn� p��padn�ch animac�?",MSB_ANONE)==2) return;
  load_side_script(script_name);
  if (delete_side(f_get_value(0,130),pos-1))
    {
    msg_box(VYMAZAT,1,"Program odm�tl vymazat grafiky ze scriptu, nastala n�jak� chyba.",MSB_OK);
    discharge_side_script();
    return;
    }
  save_side_script(script_name);
  refresh_scrs();
  }

#define ZALOZIT "Zalo�it grafiku"
static void add_graphics()
  {
  char name[200];
  int c,d;

  get_value(0,100,name);
  if (name[0]==0) {msg_box(ZALOZIT,1,"Mus�� vepsat n�jak� jm�no!",MSB_OK);return;}
  load_side_script(script_name);
  if ((c=add_side(f_get_value(0,130),name,vals(110)))==-1)
    {
    msg_box(ZALOZIT,1,"Nelze zalo�it novou grafiku! P��li� mnoho grafik, nebo nastala jin� chyba.",MSB_OK);
    discharge_side_script();
    }
  else save_side_script(script_name);
  d=vals(110);
  refresh_scrs();
  c_set_value(0,140,c+1-d+1);
  }

static char *def_nahled(int id,int x,int y,char *name)
  {
  char *fpath,*p;
  char ind=in_ddl;

  if (name==NULL) return NULL;
  concat(fpath,basic_path,name);
  in_ddl=0;
  p=get_pcx(fpath);
  in_ddl=ind;
  define(id,x,y,10,10,0,picture2);c_default((int)p);
  return p;
  }

static void nahled()
  {
  int list;
  int pos,i;
  char *p1=NULL,*p2=NULL,*p3=NULL,*p4;
  char swps=swap;

  list=f_get_value(0,130);pos=f_get_value(0,140)-1+vals(150)-1;
  if (pos<0)
    {
    msg_box("N�hled",1,"Nen� co zobrazit!",MSB_OK);
    return;
    }
  load_side_script(script_name);
  tady:
  def_dialoge(2,2,630,440,"N�hled grafiky");
  switch (list)
    {
    case 0: p1=def_nahled(10,5,25,get_side_name(list,pos,1));
            p2=def_nahled(10,255,25,get_side_name(list,pos,2));
            p3=def_nahled(10,380,25,get_side_name(list,pos,3));
            swap=1;
            break;
    case 1:
    case 2: p1=def_nahled(10,5,25,get_side_name(list,pos,1));
            swap=0;
            break;
    case 3: p1=def_nahled(10,5,25,get_side_name(list,pos,1));
            p2=def_nahled(10,255,25,get_side_name(list,pos,2));
            swap=1;
            break;
    }
  define(-1,5,5,80,20,2,button,"Ok");on_control_change(terminate_gui);
  define(110,100,5,80,20,2,button,">>");on_control_change(terminate_gui);
  define(120,190,5,80,20,2,button,"<<");on_control_change(terminate_gui);
  redraw_window();
  p4=get_side_name(list,pos,0);
  if (p4!=NULL) set_enable(0,120,p4[0]=='*' && pos>0);
  p4=get_side_name(list,pos+1,0);
  set_enable(0,110,p4!=NULL && p4[0]=='*');
  escape();
  i=o_aktual->id;
  close_current();
  free(p1);free(p2);free(p3);
  if (i==110){pos++;goto tady;}
  if (i==120){pos--;goto tady;}
  discharge_side_script();
  swap=swps;
  }

void pcxviewer()
  {
  TSTR_LIST ls;

  basic_path=get_text_field(config_file,"CESTA_GRAFIKA");
  if (basic_path==NULL)basic_path="";
  if (find_window(pcxview_win)==NULL)
     {
     CTL3D b1,b2,b3;

     ls=create_list(2);str_add(&ls,"<wait>");
     memcpy(&b1,def_border(1,0),sizeof(CTL3D));
     memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
     memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
     pcxview_win=def_window(635,440,"Prohl��e� obrazk� PCX");
     default_font=vga_font;
     memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
     on_control_change(close_pcx_window);

     define(9,10,20,110,376,0,listbox,ls,0x7fff,0);on_control_change(read_pcx);
     property(&b3,NULL,NULL,WINCOLOR);c_default(0);
     define(10,126,40,21,337,0,scroll_bar_v,0,10,1,0x0200);
     property(&b2,NULL,NULL,WINCOLOR);
     define(11,126,20,21,17,0,scroll_button,-1,0,"\x1e");
     property(&b1,icones,NULL,WINCOLOR);on_control_change(scroll_support);
     define(12,126,380,21,17,0,scroll_button,1,10,"\x1f");
     property(&b1,icones,NULL,WINCOLOR);on_control_change(scroll_support);
     define(-1,175,20,1,1,0,label,"Cesta:");
     define(20,175,32,320,12,0,input_line,240);set_default(basic_path);
     property(&b3,NULL,NULL,WINCOLOR);on_control_exit(get_directory);
     define(30,175,50,2,2,0,picture2);c_default(0);
     define(40,10,10,80,10,3,check_box,"Oto�it");c_default(swap);on_control_change(change_swap);
     define(45,10,25,160,10,3,check_box,"Grafika Skeldalu");c_default(0);on_control_change(get_directory);
     define(-1,5,80,110,75,1,label,"");property(&b2,NULL,NULL,WINCOLOR);
     define(-1,10,80,100,70,1,label,"Nov� grafika");
     define(100,10,100,100,10,1,input_line,50);set_default("");property(&b3,NULL,NULL,WINCOLOR);
     define(-1,10,115,100,12,1,label,"Sn�mk�");
     define(110,10,115,30,10,1,input_line,20,1,15,"%2d");set_default(strs(1));property(&b3,NULL,NULL,WINCOLOR);
          on_control_exit(test_int);
     define(120,10,130,60,20,1,button,"Zalo�it");property(&b1,NULL,NULL,WINCOLOR);
          on_control_change(add_graphics);
     define(130,10,160,110,50,1,radio_butts,4,"St�na","Podlaha","Strop","Oblouk");c_default(0);on_control_change(refresh_scrs);

     define(-1,5,230,110,110,1,label,"");property(&b2,NULL,NULL,WINCOLOR);
     define(-1,10,230,100,10,1,label,"Vyber grafiku:");
     define(140,10,255,100,10,1,str_line,side_names);on_enter(string_list_sup);c_default(0);property(&b2,NULL,NULL,WINCOLOR);
     define(-1,10,270,100,12,1,label,"Sn�mek");
     define(150,10,270,30,12,1,input_line,20,1,15,"%2d");set_default(strs(1));property(&b3,NULL,NULL,WINCOLOR);
          on_control_exit(test_int);
     define(160,10,300,100,10,1,check_box,"Hlavn�");c_default(0);on_control_change(clear_switchs);
     define(170,10,312,100,10,1,check_box,"Lev�");c_default(0);on_control_change(clear_switchs);
     define(180,10,324,100,10,1,check_box,"Prav�");c_default(0);on_control_change(clear_switchs);
     define(190,20,345,80,20,1,button,"P�i�adit");property(&b1,NULL,NULL,WINCOLOR);on_control_change(zmenit);
     define(200,20,370,80,20,1,button,"Vymazat");property(&b1,NULL,NULL,WINCOLOR);on_control_change(vymazat);
     define(200,20,395,80,20,1,button,"N�hled");property(&b1,NULL,NULL,WINCOLOR);on_control_change(nahled);
     define(-1,130,20,1,400,1,label,"");property(def_border(4,WINCOLOR),NULL,NULL,WINCOLOR);
     define(-1,100,30,1,1,1,label,"Tv�rce");
     define(-1,100,45,1,1,1,label,"grafick�ch");
     define(-1,100,60,1,1,1,label,"script�");
     }
  else
     {
     select_window(pcxview_win);
     }
  redraw_window();
  get_directory();
  }
