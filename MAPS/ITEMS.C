#include <types.h>
#include <mem.h>
#include <malloc.h>
#include <stdio.h>
#include <bgraph.h>
#include <devices.h>
#include <bmouse.h>
#include <event.h>
#include <gui.h>
#include <strlite.h>
#include <strlists.h>
#include <basicobj.h>
#include <memman.h>
#include "mapy.h"
#include "globals.h"
#include "edit_map.h"

#define ITEM_IKON "..\\grafika\\items\\"

//------------------------MAP EDIT ITEM SYSTEM-----------------------

#define MARK_INSIDE    0x8101
#define MARK_NEXT      0x8102
#define MARK_END_GROUP 0x8100

TITEM *item_tree=NULL;

TITEM *clean_item(TITEM *it)
  {
  memset(it,0,sizeof(*it));
  return it;
  }

TITEM *create_new_item()
  {
  if (item_tree==NULL)
     {
     item_tree=(TITEM *)getmem(sizeof(TITEM));
     item_tree->next=NULL;
     item_tree->inside=NULL;
     item_tree->parent=NULL;
     }
  else
     {
     TITEM *p;

     p=(TITEM *)getmem(sizeof(TITEM));
     p[0].next=item_tree;
     p[0].inside=NULL;
     p[0].parent=NULL;
     item_tree->parent=p;
     item_tree=p;
     }
  return item_tree;
  }

TITEM *isolate_item(TITEM *it)
  {
  TITEM *p;

  p=it->parent;
  if (it==item_tree)
     {
     item_tree=it->next;
     item_tree->parent=NULL;
     it->next=NULL;
     it->parent=NULL;
     }
  else
  if (p!=NULL)
     if (p->inside==it)
        {
        p->inside=it->next;
        it->parent=NULL;
        it->next=NULL;
        if (p->inside!=NULL)(p->inside)->parent=p;
        }
     else
        {
        p->next=it->next;
        it->parent=NULL;
        it->next=NULL;
        if (p->next!=NULL)(p->next)->parent=p;
        }
  return it;
  }

TITEM *isolate_item_group(TITEM *it)
  {
  TITEM *p;

  p=it->parent;
  if (p!=NULL)
     if (p->inside==it)
     {
     p->inside=NULL;
     it->parent=NULL;
     }
     else
     {
     p->next=NULL;
     it->parent=NULL;
     }
  return it;
  }


TITEM *insert_inside(TITEM *it1,TITEM *it2)
  {
  TITEM *p;

  if (it1->parent!=NULL) return NULL; //chyba: predmet je uz nekde vlozen - je nutne ho izolovat
  p=it2;
  while (p!=it1 && p!=NULL) p=p->parent;
  if (p==it1) return NULL; //chyba v integrite : nelze vlozit predmet do sebe;
  p=it1;
  while (p->next!=NULL) p=p->next;
  p->next=it2->inside;
  it2->inside=it1;
  it1->parent=it2;
  if (p->next!=NULL) (p->next)->parent=p;
  return it2;
  }

TITEM *insert_isolated(TITEM *it1) //vlozi izolovany predmet na zakladni uroven
  {
  if (it1->parent==NULL && it1->next==NULL)
     {
     if ((it1->next=item_tree)!=NULL) item_tree->parent=it1;
     item_tree=it1;
     return item_tree;
     }
  else
     return NULL; //predmet neni izolovan;
  }

TITEM *create_location(TITEM *it,int druh,int sector,int posit,char *level)
  {
  it->jmeno[0]='~';
  it->jmeno[1]=0;
  switch (druh)
     {
     case IT_MAPLOC:sprintf(it->popis,"MAPA: %s SECT: %d POS: %d",level,sector,posit);break;
     case IT_MONSTER:sprintf(it->popis,"MAPA: %s OBLUDA: %d UMISTENI %d",level,sector,posit);
     }
  it->druh=druh;
  it->hmotnost=sector;
  it->nosnost=posit;
  return it;
  }

void generate_item_list(TITEM *start,TSTR_LIST *list, char first)
  {
  static char c[80];
  static int start_list;

  if (first) start_list=0;
  while (start!=NULL)
     if (start->jmeno[0]!='~')
     {
     sprintf(c,"%08X %s",(int)start,start->jmeno);
     if (str_replace(list,start_list,c)==NULL) return;
     start_list++;
     if (start->inside!=NULL) generate_item_list(start->inside,list,0);
     start=start->next;
     }
  }
void generate_item_tree(TITEM *start,TSTR_LIST *list, char first)
  {
  static char tree[80];
  static char c[80];
  static int start_list;

  if (first)
     {
     start_list=0;
     tree[0]='\0';
     }
  strcat(tree,"   ");
  while (start!=NULL)
     {
     if (start->next==NULL)
        {
        char *d;

        d=strchr(tree,'\0')-3;
        strcpy(d,"�� ");
        }
     else
        {
        char *d;

        d=strchr(tree,'\0')-3;
        strcpy(d,"�� ");
        }
     if (start->jmeno[0]=='~')
         sprintf(c,"%08X %s%s",(int)start,tree,start->popis);
     else
         sprintf(c,"%08X %s%s",(int)start,tree,start->jmeno);
     if (str_replace(list,start_list,c)==NULL) return;
     start_list++;
     if (start->inside!=NULL)
        {
        if (start->next==NULL)
           {
           char *d;

           d=strchr(tree,'\0')-3;
           strcpy(d,"\xdb\xdb ");
           }
        else
           {
           char *d;

           d=strchr(tree,'\0')-3;
           strcpy(d,"�\xdb ");
           }
        generate_item_tree(start->inside,list,0);
        }
     start=start->next;
     }
  tree[strlen(tree)-3]='\0';
  }

void save_item_group(TITEM *it,FILE *f);

void save_item(TITEM *it,FILE *f)
  {
  fwrite(it,1,sizeof(*it),f);
  if (it->inside!=NULL)
     {
        {
        unsigned short i=MARK_INSIDE;

        fwrite(&i,1,sizeof(i),f);
        }
     save_item_group(it->inside,f);
     }
  if (it->next!=NULL)
     {
     unsigned short i=MARK_NEXT;

     fwrite(&i,1,sizeof(i),f);
     }
  else
     {
     unsigned short i=MARK_END_GROUP;

     fwrite(&i,1,sizeof(i),f);
     }
  }

void save_item_group(TITEM *it,FILE *f)
  {
  while (it!=NULL)
     {
     save_item(it,f);
     it=it->next;
     }
  }

TITEM *load_group(TITEM *parent,FILE *f)
  {
  TITEM *p,*q=NULL;
  unsigned short mark=MARK_NEXT;


  while (mark==MARK_NEXT)
     {
     p=(TITEM *)getmem(sizeof(TITEM));
     fread(p,1,sizeof(*p),f);
     p->next=q;
     p->parent=parent;
     p->inside=NULL;
     if (q!=NULL) q->parent=p;
     q=p;
     fread(&mark,1,sizeof(mark),f);
     if (mark==MARK_INSIDE)
       {
       p->inside=load_group(p,f);
       fread(&mark,1,sizeof(mark),f);
       }
    }
  return q;
  }

int delete_item(TITEM *it)
  {
  TITEM *p;

  if (it->parent!=NULL || it->next!=NULL) return -1;
  p=it->inside;
  while (p!=NULL)
     {
     TITEM *q;

     q=p->next;
     insert_isolated(isolate_item(p));
     p=q;
     }
  free(it);
  return 0;
  }
TITEM *clone_item(TITEM *it,char inside)
  {
  TITEM *i2,*i3,*i4,*i5,*i6;

  i2=create_new_item();
  memcpy(i2,it,sizeof(TITEM)-12);
  if (it->inside!=NULL)
     if (inside)
     {
     i4=it->inside;i5=NULL;i6=NULL;
     while (i4!=NULL)
        {
        i3=clone_item(i4,inside);
        i3=isolate_item(i3);
        if (i5==NULL)
           {
           i5=i3;
           i6=i3;
           i5->parent=i2;
           }
        else
           {
           i6->next=i3;
           i3->parent=i6;
           i6=i6->next;
           }
        i4=i4->next;
        }
     i2->inside=i5;
     }
  return i2;
  }

//--------------------------------------------------------------
void workspace_draw(int x1,int y1,int x2, int y2,OBJREC *o);

TITEM *selected_item;
int lastpos_sec;
int lastpos_sid;


void workspace_items(int x1,int y1,int x2, int y2, OBJREC *o)
  {
  int xc,yc,x,y,s,p;
  int xb1,yb1,xb2,yb2;
  char *c;
  char cn[20],dn[20];
  int mapx,mapy;
  TITEM *pt;

  workspace_draw(x1,y1,x2,y2,o);
  xc=(o->xs % M_ZOOM)>>1;
  yc=(o->ys % M_ZOOM)>>1;
  *(long *)o->data=1;
  mapx=(x2-x1)>>m_zoom;
  mapy=(y2-y1)>>m_zoom;
  pt=item_tree;
  while (pt!=NULL)
     {
     if (pt->druh==IT_MAPLOC)
        {
        c=&pt->popis;
        sscanf(c,"%s%s",dn,cn);
        s=pt->hmotnost;
        p=pt->nosnost;
        if (minfo[s].layer==cur_layer && !strcmp(cn,filename))
            {
            x=minfo[s].x;y=minfo[s].y;
            x-=xmap_offs;
            y-=ymap_offs;
            if (x>=0 && y>=0 && x<mapx && y<mapy)
              {
              xb1=xc+o->locx+x*M_ZOOM+(M_ZOOM/2)*((p & 1)!=((p & 2)>>1));
              yb1=yc+o->locy+y*M_ZOOM+(M_ZOOM/2)*((p & 2)!=0);
              xb2=xb1+M_ZOOM/2-1;
              yb2=yb1+M_ZOOM/2-1;
              curcolor=RGB555(31,0,31);
              line(xb1,yb1,xb2,yb2);
              line(xb2,yb1,xb1,yb2);
              }
            }
        }
     pt=pt->next;
     }
  }

void workspace_select_item(EVENT_MSG *msg,OBJREC *o)
  {
  MS_EVENT *ms;
  int xn,yn,xc,yc;
  int x1,y1,xp,yp;
  int s,l,i;
  char *c;
  TITEM *it;

  xc=(o->xs % M_ZOOM)>>1;
  yc=(o->ys % M_ZOOM)>>1;
  xn=o->locx+xc;
  yn=o->locy+yc;
  if (msg->msg==E_MOUSE)
     if ((ms=get_mouse(msg))->event_type==0x2)
     {
     x1=(ms->x-xn)/(M_ZOOM/2);
     y1=(ms->y-yn)/(M_ZOOM/2);
     xp=x1 & 0x1;x1>>=1;
     yp=y1 & 0x1;y1>>=1;
     x1+=xmap_offs;
     y1+=ymap_offs;
     l=2*(yp!=0)+(xp!=yp);
     for (i=0;i<maplen;i++)
        if (minfo[i].x==x1 && minfo[i].y==y1)
           {
           lastpos_sec=i;
           lastpos_sid=l;
           it=item_tree;
           while (it!=NULL)
              {
              if (it->druh==IT_MAPLOC)
                 {
                 char cn[20],dn[20];
                 int lc;

                 c=&it->popis;
                 sscanf(c,"%s%s%s%d%s%d",dn,cn,dn,&s,dn,&lc);
                 if (s==i && lc==l && !strcmp(cn,filename))
                    {
                    selected_item=it;
                    return;
                    }
                 }
              it=it->next;
              }
           }
     selected_item=NULL;
     }
  }

TITEM *items_on_map()
  {
  OBJREC *o;

  waktual->modal=0;
  create_map_win(-1);
  select_window(tool_bar);
  if (waktual->id==tool_bar) close_current();
  select_window(map_win);
  o=find_object(waktual,10);
  o->call_draw=workspace_items;
  o->call_event=workspace_select_item;
  set_window_modal();
  selected_item=NULL;
  while (selected_item==NULL && waktual->id==map_win) do_events();
  if (waktual->id==map_win) close_current();
  waktual->modal=1;
  return selected_item;
  }

void item_umisti_v_mape(TITEM *it)
  {
  OBJREC *o;

  waktual->modal=0;
  create_map_win(-1);
  select_window(tool_bar);
  if (waktual->id==tool_bar) close_current();
  select_window(map_win);
  o=find_object(waktual,10);
  o->call_draw=workspace_items;
  o->call_event=workspace_select_item;
  set_window_modal();
  lastpos_sec=-1;
  while (waktual->id==map_win)
     {
     do_events();
     if (lastpos_sec!=-1)
        {
        if (selected_item!=NULL) insert_inside(isolate_item(clone_item(it,0)),selected_item);
        else insert_inside(isolate_item(clone_item(it,1)),create_location(create_new_item(),IT_MAPLOC,lastpos_sec,lastpos_sid,filename));
        lastpos_sec=-1;
        redraw_desktop();
        }
     }
  waktual->modal=1;
  return;
  }

void ikona_display(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  static char *ikonlib=NULL;
  static int lastikolib=-1;
  int nowlib;
  int ikn;
  char s[200];

  ikn=*(int *)o->data;
  nowlib=ikn/18;
  if (lastikolib!=nowlib)
     {
     FILE *f;int i;
     sprintf(s,ITEM_IKON"ikony%02d.lib",nowlib);
     f=fopen(s,"rb");
     if (f==NULL)
        for(i=0;i<18;i++)
           {
           memset(&ikonlib[2+2+2+512+2475*i],0,2993);
           }
       else
        {
        if (ikonlib==NULL) ikonlib=(char *)getmem(18*2993);
        fread(ikonlib,1,18*2993,f);
        fclose(f);
        }
     lastikolib=nowlib;
     }
  ikn%=18;
  bar(x1,y1,x2,y2);
  put_picture(x1,y1,ikonlib+ikn*2993);
  }

void ikona_click(EVENT_MSG *msg,OBJREC *o)
  {
  MS_EVENT *ms;

  o;
  if (msg->msg==E_MOUSE)
     {
     ms=get_mouse(msg);
     if (ms->event_type==0x4)
         set_change();
     }
  }

void ikona(OBJREC *o)
  {
  o->call_draw=ikona_display;
  o->call_event=ikona_click;
  o->datasize=4;
  }

int select_ikon_lib(int last)
  {
  int x,y,i,s;

  s=last;
  def_dialoge(50,200,500,264,"Knihovna ikon - vyber jednu ikonu");
  x=5;y=20;
  for (i=0;i<30;i++)
     {
     define(i+10,x,y,45,55,0,ikona);c_default(last+i);
     property(def_border(5,WINCOLOR),NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
     x+=48;if (x+48>490) {y+=58;x=5;}
     }
  define(300,10,10,80,20,3,button,"<<");
  property(def_border(1,0),NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(310,10,10,80,20,2,button,">>");
  property(def_border(1,0),NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(320,175-40,10,80,20,3,button,"Zru�it");
  property(def_border(1,0),NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_window();
  do
     {
     escape();
     if (o_aktual->id==300)
     {
     last-=30;if (last<0) last=0;
     for(i=0;i<30;i++) c_set_value(0,10+i,last+i);
     }
  if (o_aktual->id==310)
     {
     last+=30;
     for(i=0;i<30;i++) c_set_value(0,10+i,last+i);
     }
     }
  while (o_aktual->id==310 || o_aktual->id==300);
  i=o_aktual->id;
  close_current();
  if (i==320) i=s; else i=i-10+last;
  return i;
  }

void change_item_ikone()
  {
  c_set_value(0,130,select_ikon_lib(f_get_value(0,130)));
  }

char nvlast[][16]=
  {"cS�la","Schopnost magie","Pohyblivost","Obratnost","Max zran�n�",
  "Kondice","Max mana","Obrana(doln�)","Obrana(Horn�)","�tok(Doln�)",
  "�tok(Horn�)","Ohe�","Voda","Zem�","Vzduch","Mysl","Magick� s�la(D)",
  "Magick� s�la(H)","\0"};

void ukaz_vlastnosti(int pocet,int x,int y,int id,short *it)
  {
  word i,ts;

  i=0;
  curfont=vga_font;
  default_font=curfont;
  while (nvlast[i][0] && i<pocet)
     {
     ts=text_width(nvlast[i]);
     if (y+15>200)
        {
        x+=200;
        y=20;
        }
     define(-1,x,y,ts,10,0,label,nvlast[i]);
     define(id++,x+130,y,60,12,0,input_line,7,-32767,+32767,"%6d");
     property(def_border(5,WINCOLOR),NULL,NULL,WINCOLOR);
     set_default(strs(*it));it++;
     on_exit(test_int);
     y+=15;
     i++;
     }
  }

void oprav_vlastnosti(TITEM *it)
  {
  int i;
  def_dialoge(120,80,410,300,"Vlastnosti");
  ukaz_vlastnosti(4,5,20,70,&it->podminky);
  define(-1,5,93,190,0,0,label,"");property(def_border(2,WINCOLOR),NULL,NULL,WINCOLOR);
  ukaz_vlastnosti(99,5,105,80,&it->zmeny);
  define(130,5,200,100,10,0,check_box,"Na hlavu");
  define(140,5,212,100,10,0,check_box,"Na krk");
  define(150,5,224,100,10,0,check_box,"Na telo");
  define(160,5,236,100,10,0,check_box,"Na ruce");
  define(170,5,248,100,10,0,check_box,"Na lokte");
  define(180,5,260,100,10,0,check_box,"Kolem pasu");
  define(190,5,272,100,10,0,check_box,"Na nohy");
  define(200,105,200,100,10,0,check_box,"Na chodidla");
  define(210,105,212,100,10,0,check_box,"Na ramena");
  define(220,105,224,100,10,0,check_box,"Na z�da");
  define(300,10,10,80,20,2,button,"Zru�it");on_control_change(terminate_gui);
   property(def_border(1,0),NULL,NULL,WINCOLOR);
  define(310,10,40,80,20,2,button,"Ok");on_control_change(terminate_gui);
   property(def_border(1,0),NULL,NULL,WINCOLOR);
  define(-1,225,206,70,10,0,label,"Magick� �tok");
  define(250,225,218,70,70,0,radio_butts,5,"Ohe�","Voda","Zem�","Vzduch","Mysl");
  c_default(it->zmeny[VLS_MGZIVEL]);
  fill_bit_fields(0,130,it->place_map,11);
  redraw_window();
  escape();
  if (o_aktual->id==310)
     {
     for(i=0;i<4;i++) it->podminky[i]=vals(i+70);
     for(i=0;i<18;i++) it->zmeny[i]=vals(i+80);
     it->place_map=get_bit_fields(0,130,10);
     it->zmeny[VLS_MGZIVEL]=f_get_value(0,250);
     }
  close_current();
  }

void otevri_vlastnosti()
  {
  TITEM *it;

  get_value(0,5,&it);
  oprav_vlastnosti(it);
  }

void save_edited_item()
  {
  TITEM *it;

  get_value(0,5,&it);
  get_value(0,15,it->jmeno);
  get_value(0,20,it->popis);
  it->hmotnost=vals(40);
  it->nosnost=vals(50);
  it->keynum=vals(60);
  it->user_value=vals(70);
  it->use_event=vals(80);
  it->druh=f_get_value(0,90);
  it->spell=vals(100);
  it->magie=vals(110);
  it->silaspell=vals(120);
  it->ikona=f_get_value(0,130);
  terminate_gui();
  }

TITEM *select_item(TITEM *it);

void e_item_insert_inside()
  {
  TITEM *it,*is;

  save_edited_item();
  get_value(0,5,&it);
  is=select_item(item_tree);
  if (is==NULL) return;
  it=isolate_item(it);
  if (insert_inside(it,is)==NULL)
     {
     msg_box("Chyba integrity:",'\x1',"Nelze vlo�it tento p�edm�t do zvolen�ho "
              "Patrn� se pokou��� vlo�it tut�� v�c samu do sebe. Nebo v�c nen� "
              "izolov�na, nebo ji nelze izolovat. Je mo�n� �e se sna��� vlo�it "
              "v�c do v�ci, kter� je uvnit� t�to v�ci, nebo v uvnit� v�ci, kter� "
              "je uvnit� v�ci, do kter� chce� v�c um�stit atd. Mohlo by dojit k "
              "zacyklen� a to nelze dopustit. Program bude v�c izolovat, jeliko� "
              "ji� nelze rekonstruovat p�vodn� stav","Budu si pamatovat",NULL);
     insert_isolated(it);
     return;
     }
  terminate_gui();
  }

void e_item_insert_map()
  {
  TITEM *it;

  save_edited_item();
  get_value(0,5,&it);
  item_umisti_v_mape(it);
  }
void item_edit(TITEM *it)
  {
  CTL3D *c;

  c=def_border(5,WINCOLOR);
  def_dialoge(120,80,410,300,"Oprava �daj� o p�edm�tu");
  curfont=vga_font;
  default_font=curfont;
  define(-1,5,20,50,10,0,label,"Jm�no (31 znak�)");
  define(15,10,35,288,12,0,input_line,31);set_default(it->jmeno);
   property(def_border(2,WINCOLOR),NULL,flat_color(RGB555(15,0,0)),WINCOLOR);
  define(-1,5,50,50,10,0,label,"Popis (63 znak�)");
  define(20,10,65,380,12,0,input_line,63);set_default(it->popis);
   property(def_border(2,WINCOLOR),NULL,flat_color(0xF),WINCOLOR);
  define(5,0,0,0,0,0,value_store,4);set_default(&it);
  define(30,10,30,80,20,1,button,"Vlastnosti");
   property(def_border(1,0),NULL,NULL,WINCOLOR);on_control_change(otevri_vlastnosti);
  define(-1,5,80,100,10,0,label,"Hmotnost");
  define(-1,5,95,100,10,0,label,"Nosnost");
  define(-1,5,110,100,10,0,label,"ID Kl��e");
  define(-1,5,125,100,10,0,label,"?Hodnota");
  define(-1,5,140,100,10,0,label,"�.Ud�losti");
  define(-1,5,155,100,10,0,label,"Druh");
  def_border(5,WINCOLOR);
  define(40,105,80,60,12,0,input_line,7,-32767,+32767,"%6d");
   property(c,NULL,NULL,WINCOLOR);on_exit(test_int);set_default(strs(it->hmotnost));
  define(50,105,95,60,12,0,input_line,7,-32767,+32767,"%6d");
   property(c,NULL,NULL,WINCOLOR);on_exit(test_int);set_default(strs(it->nosnost));
  define(60,105,110,60,12,0,input_line,7,-32767,+32767,"%6d");
   property(c,NULL,NULL,WINCOLOR);on_exit(test_int);set_default(strs(it->keynum));
  define(70,105,125,60,12,0,input_line,7,-32767,+32767,"%6d");
   property(c,NULL,NULL,WINCOLOR);on_exit(test_int);set_default(strs(it->user_value));
  define(80,105,140,60,12,0,input_line,7,-32767,+32767,"%6d");
   property(c,NULL,NULL,WINCOLOR);on_exit(test_int);set_default(strs(it->use_event));
  define(90,80,155,120,108,0,radio_butts,10,
      "Nespecif.",
      "Brn�n�",
      "Ru�n� zbra�",
      "Svitek/Hulka",
      "St�eln� zbr.",
      "Vrhac�",
      "Lektvar/M�ch",
      "J�dlo",
      "Batoh/Truhla",
      "Specialn�");c_default(it->druh);
  define(-1,105,80,100,10,1,label,"Kouzlo:");
  define(-1,105,95,100,10,1,label,"Mana celk:");
  define(-1,105,110,100,10,1,label,"S�la kouzla");
  define(100,55,80,60,12,1,input_line,7,-32767,32767,"%6d");on_exit(test_int);
   property(c,NULL,NULL,WINCOLOR);set_default(strs(it->spell));
  define(110,55,95,60,12,1,input_line,7,-32767,32767,"%6d");on_exit(test_int);
   property(c,NULL,NULL,WINCOLOR);set_default(strs(it->magie));
  define(120,55,110,60,12,1,input_line,7,-32767,32767,"%6d");on_exit(test_int);
   property(c,NULL,NULL,WINCOLOR);set_default(strs(it->silaspell));
  define(130,10,180,45,55,0,ikona);c_default(it->ikona);
   property(c,NULL,NULL,WINCOLOR);on_control_change(change_item_ikone);
  define(300,10,10,80,20,3,button,"Ulo�");property(def_border(1,0),NULL,NULL,RGB(0,24,0));on_control_change(save_edited_item);
  define(310,100,10,80,20,3,button,"Vlo�it do");property(def_border(1,0),NULL,NULL,WINCOLOR);on_control_change(e_item_insert_inside);
  define(320,100,10,80,20,2,button,"Um�stit");property(def_border(1,0),NULL,NULL,WINCOLOR);on_control_change(e_item_insert_map);
  define(330,10,10,80,20,2,button,"Zru�it");property(def_border(1,0),NULL,flat_color(RGB555(31,31,31)),RGB555(16,0,0);on_control_change(terminate_gui);
  redraw_window();
  escape();
  close_current();
  }


void select_item_edit()
  {
  TITEM *it;
  TSTR_LIST ls;
  int i;

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  i=f_get_value(0,9);
  if (ls[i]==NULL) return;
  sscanf(ls[i],"%X",&it);
  if ((unsigned short)it->druh>=256)
     {
     msg_box("Nelze!",'\x1',"Nelze upravovat tento druh v�ci","Beru na v�dom�",NULL);
     return;
     }
  item_edit(it);
  generate_item_tree(item_tree,&ls,1);
  str_delfreelines(&ls);
  send_message(E_GUI,9,E_CONTROL,1,ls);
  }

void select_item_klon()
  {
  TITEM *it;
  TSTR_LIST ls;
  int i;

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  i=f_get_value(0,9);
  if (ls[i]==NULL) return;
  sscanf(ls[i],"%X",&it);
  if ((unsigned short)it->druh>=256)
     {
     msg_box("Nelze!",'\x1',"Nelze klonovat tento druh v�ci","Beru na v�dom�",NULL);
     return;
     }
  if (it->inside!=NULL)
     {
     i=msg_box("Klonovat",'\x2',"Klonovat i v�ci uvnit�?","Ano","Ne",NULL);
     }
  it=clone_item(it,i==1);
  generate_item_tree(item_tree,&ls,1);
  str_delfreelines(&ls);
  send_message(E_GUI,9,E_CONTROL,1,ls);
  }

void select_item_isolate()
  {
  TITEM *it;
  TSTR_LIST ls;
  int i;

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  i=f_get_value(0,9);
  if (ls[i]==NULL) return;
  sscanf(ls[i],"%X",&it);
  isolate_item(it);
  insert_isolated(it);
  generate_item_tree(item_tree,&ls,1);
  str_delfreelines(&ls);
  send_message(E_GUI,9,E_CONTROL,1,ls);
  }

void check_for_empty_loc()
  {
  TITEM *it1,*it2;

  it1=item_tree;
  while (it1!=NULL)
     {
     if ((it1->druh==IT_MAPLOC || it1->druh==IT_MONSTER) && it1->inside==NULL)
        {
        it2=it1;it1=it2->next;
        isolate_item(it2);
        delete_item(it2);
        }
     else
        it1=it1->next;
     }
  }

void select_item_delete()
  {
  TITEM *it;
  TSTR_LIST ls;
  int i;

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  i=f_get_value(0,9);
  if (ls[i]==NULL) return;
  sscanf(ls[i],"%X",&it);
  if (item_tree==it && it->next==NULL)
     {
     msg_box("Nelze!",'\x1',"Jeden p�edm�t mus� z�stat","Ok",NULL);
     return;
     }
  if (msg_box("P�edm�ty",'\x2',"Vymazat vybranou v�c","Souhlas","Ne",NULL)==2) return;
  isolate_item(it);
  delete_item(it);
  check_for_empty_loc();
  release_list(ls);
  ls=create_list(16);
  generate_item_tree(item_tree,&ls,1);
  str_delfreelines(&ls);
  send_message(E_GUI,9,E_CONTROL,1,ls);
  set_value(0,5,item_tree);
  }

void items_on_map_show()
  {
  TITEM *it;
  TSTR_LIST il;
  int ls;
  int i;

  send_message(E_GUI,9,E_CONTROL,0,&il);
  i=f_get_value(0,9);
  if (il[i]==NULL) return;
  sscanf(il[i],"%X",&it);
  unselect_map();
  if (it->druh==IT_MAPLOC)
     {
     char c[20],d[20];
     sscanf(it->popis,"%s%s",c,d);
     if (!strcmp(d,filename)) minfo[it->hmotnost].flags|=0x1;
     }
  it=items_on_map();
  if (it==NULL) return;
  ls=str_count(il);
  for(i=0;i<ls;i++)
     if (il[i]!=NULL)
     {
     TITEM *q;
     sscanf(il[i],"%x",&q);
     if (q==it) break;
     }
  if (il[i]!=NULL)
     {
     c_set_value(0,9,i);
     send_message(E_GUI,9,E_CONTROL,2);
     }
  }

TITEM *select_item(TITEM *it)
  {
  CTL3D b2;
  TSTR_LIST il;
  char *c;
  int cis;
  int i,ls;

  il=create_list(16);
  generate_item_tree(item_tree,&il,1);
  str_delfreelines(&il);
  ls=str_count(il);
  for(i=0;i<ls;i++)
     if (il[i]!=NULL)
     {
     TITEM *q;
     sscanf(il[i],"%x",&q);
     if (q==it) break;
     }
  if (i==ls) i=0;
  def_dialoge(120,80,410,300,"P�edm�ty");
  default_font=vga_font;
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  define(9,5,20,380,210,0,listbox,il,RGB555(31,31,31),9);
   property(def_border(1,0),NULL,NULL,WINCOLOR);c_default(i);
  define(10,1,40,19,171,1,scroll_bar_v,0,10,1,SCROLLBARCOL);
  property(&b2,NULL,NULL,WINCOLOR);c_default(0);c_default(i);
  define(11,1,20,21,17,1,scroll_button,-1,0,"\x1e");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(12,1,213,21,17,1,scroll_button,1,10,"\x1f");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(5,0,0,0,0,0,value_store,4);set_default(&it);
  define(20,5,5,80,15,3,button,"Upravit");on_control_change(select_item_edit);
   property(def_border(1,0),NULL,NULL,WINCOLOR);
  define(30,5,25,80,15,3,button,"Klonovat");on_control_change(select_item_klon);
   property(def_border(1,0),NULL,NULL,WINCOLOR);
  define(40,5,45,80,15,3,button,"Vymazat");on_control_change(select_item_delete);
   property(def_border(1,0),NULL,NULL,WINCOLOR);
  define(50,5,5,80,15,2,button,"Zru�it");on_control_change(terminate_gui);
   property(def_border(1,0),NULL,NULL,WINCOLOR);
  define(60,5,25,80,15,2,button,"Vyber");on_control_change(terminate_gui);
   property(def_border(1,0),NULL,NULL,WINCOLOR);
  define(70,90,5,80,15,3,button,"Izoluj");on_control_change(select_item_isolate);
   property(def_border(1,0),NULL,NULL,WINCOLOR);
  define(80,90,25,80,15,3,button,"Mapa");on_control_change(items_on_map_show);
   property(def_border(1,0),NULL,NULL,WINCOLOR);
  redraw_window();
  escape();
  it=NULL;
  if (o_aktual->id==60)
     {
     cis=f_get_value(0,9);
     send_message(E_GUI,9,E_CONTROL,0,&il);
     c=il[cis];
     if (c!=NULL) sscanf(c,"%x",&it);
     }
  close_current();
  return it;
  }

void editor_veci()
  {
  if (item_tree==NULL) item_edit(clean_item(create_new_item()));
  select_item(item_tree);
  }

void save_items()
  {
  FILE *f;

  if (item_tree==NULL) return;
  f=fopen(ITEM_FILE,"wb");
  save_item_group(item_tree,f);
  fclose(f);
  }

void load_items()
  {
  FILE *f;

  f=fopen(ITEM_FILE,"rb");
  if (f==NULL) return;
  item_tree=load_group(NULL,f);
  fclose(f);
  }
