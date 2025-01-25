#include <skeldal_win.h>
#include <dos.h>
#include <types.h>
#include <mem.h>
#include <malloc.h>
#include <memman.h>
#include <stdio.h>
#include <bgraph.h>
#include <event.h>
#include <devices.h>
#include <bmouse.h>
#include <gui.h>
#include <strlite.h>
#include <strlists.h>
#include <wav.h>
#include <zvuk.h>
#include "mapy.h"
#include <basicobj.h>
#include "globals.h"
#include "edit_map.h"
#include "save_map.h"
#include "steny.h"
#include "mob_edit.h"
#include <pcx.h>



char *mob_dir;
TSTR_LIST enemy_list=NULL;
TSTR_LIST enemy_sound=NULL;
TMOB moblist[TOTAL_MOBS];
short mob_map[MAPSIZE];
int enemy_win=-1;

void edit_mob(int i);

static void mirror_pcx(word *pcx)
  {
  int xs;
  int ys;
  int i,j,k,l;
  char *c,*d,*e;

  xs=pcx[0];
  ys=pcx[1];
  pcx+=3+256;
  l=xs/2;
  c=(char *)pcx;
  for(i=0;i<ys;i++,c+=xs)
     for(j=0,d=c,e=c+xs-1;j<l;j++,d++,e--)
        {
        k=*d;*d=*e;*e=k;
        }
  }

void pcx_view_init(OBJREC *o)
  {
  o->userptr=getmem(129+4);
  memset(o->userptr,0,129+4);
  }

void pcx_view_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  char *oldname;
  char *newname;
  void *picture;
  char mirror=0;


  picture=*(void **)(oldname=o->userptr);
  oldname+=4;
  newname=o->data;
  if (newname[0]=='*')
     newname++,mirror=1;
  if (strcmp(oldname,newname))
     {
     free(picture);
     picture=NULL;
     }
  if (picture==NULL)
     {
     open_pcx(newname,A_8BIT,&picture);
     if (mirror && picture!=NULL) mirror_pcx(picture);
     }
  if (picture==NULL)
     {
     char *d=strrchr(newname,'\\');
     void *z;long s;
     if (d!=NULL) d++;else d=newname;
     z=afile(d,read_group(0),&s);
     if  (z!=NULL) load_pcx(z,s,A_8BIT,&picture);free(z);
     if (mirror) mirror_pcx(picture);
     }
  bar(x1,y1,x2,y2);
  if (picture!=NULL) put_picture(x1,y1,picture);
  *(void **)(o->userptr)=picture;
  strcpy(oldname,newname);
  }

void pcx_view_done(OBJREC *o)
  {
  void *picture;
  picture=*(void **)(o->userptr);
  free(picture);
  }

void pcx_view(OBJREC *o)
  {
  o->runs[0]=pcx_view_init;
  o->runs[1]=pcx_view_draw;
  o->runs[3]=pcx_view_done;
  o->datasize=129;
  }

void symetry_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  x2;
  curcolor=RGB555(31,31,31);
  x1+=*(int *)o->data;
  ver_line_xor(x1,y1,y2);
  }

void symetry_event(EVENT_MSG *msg,OBJREC *o)
  {
  MS_EVENT *ms;

  if (msg->msg==E_MOUSE)
     {
     int x1,x2;
     ms=get_mouse(msg);
     if (ms->tl1)
        {
        x1=*(int *)o->data+o->locx;
        curcolor=RGB555(31,31,31);
        schovej_mysku();
        ver_line_xor(x1,o->locy,o->locy+o->ys);
        x2=ms->x;
        ver_line_xor(x2,o->locy,o->locy+o->ys);
        ukaz_mysku();
        if (x1<x2) showview(x1,o->locy,x2+1,o->locy+o->ys);
        else showview(x2,o->locy,x1+1,o->locy+o->ys);
        *(int *)o->data=x2-o->locx;
        }
     }
  }

void symetry(OBJREC *o)
  {
  o->runs[1]=symetry_draw;
  o->runs[2]=symetry_event;
  o->datasize=4;
  }

#define delete_mob(i) memset(&moblist[i],0,sizeof(TMOB);moblist[i].cislo_vzoru=-1;create_enemy_list()
#define clear_mob_map() memset(mob_map,0xff,sizeof(mob_map))
#define clear_map_place(i) (mob_map[i]=-1)
#define place_mob(i,j) (mob_map[i]=(j))

void purge_mob_map()
  {
  clear_mob_map();
  }


int add_mob_sound(char *wav)
  {
  int i,c;
  if (enemy_sound==NULL)
     {
     enemy_sound=create_list(4);
     return str_add(&enemy_sound,wav);
     }
  else
     {
     c=str_count(enemy_sound);
     for(i=0;i<c;i++)
        if (enemy_sound[i]!=NULL)
           if (!strcmp(enemy_sound[i],wav)) return i;
     return str_add(&enemy_sound,wav);
     }
  }

void check_unused_sounds()
  {
  char *block;
  int c,i,j,z;

  if (enemy_sound==NULL) return;
  c=str_count(enemy_sound);
  block=(char *)getmem(c);
  memset(block,1,c);
  for(i=0;i<TOTAL_MOBS;i++)
     if (moblist[i].cislo_vzoru!=-1)
      for(j=0;j<MOB_SOUNDS;j++)
      {
      z=moblist[i].sounds[j];
      if (z && z<=c) block[z-1]=0;
      }
  for(i=0;i<max_items;i++) if (item_list[i].sound) block[item_list[i].sound-1]=0;
  for(i=0;i<c;i++)
     if (block[i]) str_remove(&enemy_sound,i);
  }


void init_mob_list()
  {
  int i;

  memset(moblist,0,sizeof(moblist));
  for(i=0;i<TOTAL_MOBS;i++)moblist[i].cislo_vzoru=-1;
  clear_mob_map();
  }

int create_mob()
  {
  int i;
  for(i=0;i<TOTAL_MOBS;i++)
     if (moblist[i].cislo_vzoru==-1) return i;
  return -1;
  }

void create_enemy_list()
  {
  int i,mob_total;

  if (enemy_list!=NULL) release_list(enemy_list);
  enemy_list=NULL;
  mob_total=0;
  for(i=0;i<TOTAL_MOBS;i++)if (moblist[i].cislo_vzoru!=-1) mob_total++;
  enemy_list=create_list(mob_total+2);
  if (mob_total)
     for(i=0;i<TOTAL_MOBS;i++)if (moblist[i].cislo_vzoru!=-1)
        {
        char s[50];
        sprintf(s,"%3d %s",i,moblist[i].name);
        str_add(&enemy_list,s);
        }
     else;
   str_add(&enemy_list," -1 <nov� potvora>");
  }


void new_mob()
  {
  int i,j;
  char c;


  if (!mglob.local_monsters && check_data_password()==0) return;
  i=f_get_value(0,29);
  c=enemy_list[i]!=NULL && moblist[i].cislo_vzoru!=-1;
  j=create_mob();
  if (c)
     {
     sscanf(enemy_list[i],"%d",&i);
     if (i!=-1) moblist[j]=moblist[i];
     }
  else
     {
     memset(moblist+i,0,sizeof(TMOB));
     moblist[i].cislo_vzoru=-1;
     }
  edit_mob(j);
  create_enemy_list();
  send_message(E_GUI,29,E_CONTROL,1,enemy_list);
  }

static char nvlast[][16]=
  {"S�la","UM(%kouzlen�)","Pohyblivost","Obratnost","Max zran�n�",
  "","","Obrana(doln�)","Obrana(Horn�)","�tok(Doln�)",
  "�tok(Horn�)","Ohe�","Voda","Zem�","Vzduch","Mysl","�ivoty Regen",
  "","","Magick� s�la(D)", "Magick� s�la(H)","","��innek z�sahu","*"};


char smery_anim[][7]=
  {"Vp�ed","Vlevo","Vzad","Vpravo","�tok","Z�sah"};
char smery_znaky[]="FLBRCH";

char sekvence[]="0123456789ABCDEF";

char load_ddl_seq(char *name,char otoceni)
  {
  char *z,*c;
  long size;

     c=z=afile(name,9,&size);
     if  (z!=NULL)
        {
        while (otoceni--) if (z!=NULL) z=strchr(z+1,'\n');
        if (z==NULL)
           {
           free(c);
           return 0;
           }
        while (*z<32) z++;
        memcpy(sekvence,z,sizeof(sekvence));
        }
     free(c);
     return 1;
  }

char load_sequence(char *mobname,int otoceni)
  {
  char s[128];
  char st[13];
  FILE *f;
  int i,c;

  sprintf(s,"%s%s.SEQ",MOB_DIR,mobname);
  sprintf(st,"%s.SEQ",mobname);
  f=fopen(s,"r");
  if (f==NULL)
     {
     char text[256];

     if (load_ddl_seq(st,otoceni)) return 0;
     sprintf(text,"Nemohu nalezt soubor %s  jen� ma obsahovat sekvence animac� pro tuto potvoru...",s);
     msg_box("Upozorn�n�!",'\x1',text,"Beru na v�dom�",NULL);
     return 1;
     }
  while(otoceni--)
     while (fgetc(f)!='\n');
  c=fgetc(f); i=0;
  while (c!=EOF && c!='\n' && i<sizeof(sekvence))
     {
     sekvence[i++]=c;
     c=fgetc(f);
     }
  fclose(f);
  if (c==EOF)
     {
     char text[256];

     sprintf(text,"Soubor %s obsahuje chybn� �daje",s);
     msg_box("Upozorn�n�!",'\x1',text,"Beru na v�dom�",NULL);
     return 1;
     }
  return 0;
  }

void pohyb_moba(EVENT_MSG *msg,void **data)
  {
  static int phase;
  static char turn;
  static int delay=0;
  char ch;


  data;
  switch (msg->msg)
     {
     case E_INIT:phase=-1;
                 turn=f_get_value(0,185);
                 break;
     case E_TIMER:if (delay) delay--;
              else
              {
              char name[129],c[10];
              word *picture;
              int x,y,xs,ys;

              delay=2;
              get_value(0,100,c);
              sprintf(name,"%s%s%c%c.PCX",MOB_DIR,c,smery_znaky[turn]=='R'?'L':smery_znaky[turn],sekvence[phase+1]);
              x=320;y=0;xs=1;ys=1;
              ch=open_pcx(name,A_8BIT,(void **)&picture);
              if (ch)
                 {
                 void *z;long s;char *c;
                 c=strrchr(name,'\\');if (c==NULL) c=name;else c++;
                 z=afile(c,9,&s);
                 if (z!=NULL) load_pcx(z,s,A_8BIT,(void **)&picture),free(z),ch=0;else ch=1,picture=NULL;
                 }
              if (!ch)
                 {
	         if (turn==3)
		 	mirror_pcx(picture);
                 x-=f_get_value(0,300+turn*16+phase+1);
                 xs=picture[0];
                 ys=picture[1];
                 y=460-ys;
		 if (x<0) x=0;
                 put_picture(x,y,picture);
                 free(picture);
                 }
              phase=(phase+1)%vals(120+turn);
              showview(0,0,0,0);
              curcolor=0;
              bar(x,y,x+xs,y+ys);
              }
              break;
     case E_KEYBOARD:exit_wait=1;break;
     case E_MOUSE:
              {
              MS_EVENT *ms;

              ms=get_mouse(msg);
              if (ms->event_type & 0x4) exit_wait=1;
              msg->msg=-1;
              }
     }
  }

void play_mob()
  {
  int i;
  char s[12];

  int mx =  DxGetResX() - 1;
  int my =  DxGetResY() - 1;

  get_value(0,100,s);
  i=f_get_value(0,185);
  if (load_sequence(s,i)) return;
  update_mysky();
  schovej_mysku();
  curcolor=0;
  bar(0,0,mx,my);
  send_message(E_ADD,E_TIMER,pohyb_moba);
  send_message(E_ADD,E_KEYBOARD,pohyb_moba);
  send_message(E_ADD,E_MOUSE,pohyb_moba);
  escape();
  send_message(E_DONE,E_TIMER,pohyb_moba);
  send_message(E_DONE,E_KEYBOARD,pohyb_moba);
  send_message(E_DONE,E_MOUSE,pohyb_moba);
  redraw_desktop();
  ukaz_mysku();
  }


void build_mobs_name()
  {
  int i,j,k;
  char c,path[129],name[10];
  char *d;
  char m;

  i=f_get_value(0,180);
  j=f_get_value(0,185);
  get_value(0,100,name);
  c=smery_znaky[j];
  if (c=='R') c='L',m=1;else m=0;
  sprintf(path,"%s%s%s%c%c.PCX",m?"*":"",MOB_DIR,name,c,sekvence[i]);
  k=f_get_value(0,205);
  c_set_value(0,205,300);
  c_set_value(0,205,k);
  c_set_value(0,205,300);
  set_value(0,200,path);
  c_set_value(0,205,f_get_value(0,300+i+16*j));
  d=strrchr(path,'\\');if (d!=NULL) d++;else d=path;
  set_value(0,207,d);
  }

void check_mobs_name()
  {
  char s[10];
  int i;

  get_value(0,100,s);
  for(i=0;i<6 && s[i];i++) if (s[i]==32) s[i]='_';
  for(;i<6;i++) s[i]='_';
  s[i]=0;
  strupr(s);
  set_value(0,100,s);
  c_set_value(0,180,0);
  c_set_value(0,185,0);
  build_mobs_name();
  }

void mob_next()
  {
  int i,j,max;
  char s[12];
  i=f_get_value(0,180);
  j=f_get_value(0,185);
  get_value(0,100,s);
  load_sequence(s,j);
  c_set_value(0,300+i+16*j,f_get_value(0,205));
  max=vals(120+j);
  i=(i+1)%(max+1);
  c_set_value(0,180,i);
  build_mobs_name();
  redraw_desktop();
  }

void mob_turn()
  {
  int i,j;
  char s[12];
  i=f_get_value(0,180);
  j=f_get_value(0,185);
  c_set_value(0,300+i+16*j,f_get_value(0,205));
  do
     j=(j+1)%+6;
  while (!vals(120+j));
  c_set_value(0,185,j);
  c_set_value(0,180,0);
  get_value(0,100,s);
  load_sequence(s,j);
  build_mobs_name();
  redraw_desktop();
  }

void mob_ask_delete()
  {

  if (msg_box("Dotaz?",'\x2',"Opravdu chce� potovoru vymazat ze seznam� vzor�? Ujisti se, �e tuto potvoru nepou��v�� v ��dn� jin� map�","Ano","Ne",NULL)==1)
     terminate_gui();
  o_aktual=find_object(waktual,260);
  }

char *sound_ev[]=
  {"P�i ch�zi","�tok","Z�sah","Reserved"};

void mob_test_sound()
  {
  char *c;
  TSTR_LIST ls;
  int d;

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  d=f_get_value(0,9);
  if (ls[d]==NULL) return;
  concat(c,"",ls[d]);
  name_conv(c);
  add_task(16384,play_wav,c,0);
  do_events();
  do_events();
  }

void mob_sound_copy()
  {
  int id=o_aktual->id;
  TSTR_LIST ls;
  int d;
  char c[20];

  send_message(E_GUI,9,E_CONTROL,0,&ls);
  d=f_get_value(0,9);
  if (ls[d]==NULL) return;
  strncpy(c,ls[d],20);name_conv(c);
  id-=20;
  set_value(0,id,c);
  }


void mob_inv()
  {
  int m,i;
  CTL3D b1;
  TSTR_LIST ls_sorts=NULL;

  create_isort_list(&ls_sorts,-1);
  m=f_get_value(0,190);
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,50,400,390,"Invent�� potvory");
  str_insline(&ls_sorts,0,"<nic>");
  for(i=0;i<MOBS_INV;i++)
     {
     define(i+10,10,20+i*15,250,10,0,str_line,ls_sorts);on_enter(string_list_sup);
     c_default(moblist[m].inv[i]);
     }
  define(300,5,20,80,20,1,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(310,5,45,80,20,1,button,"Zru�it");property(&b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_desktop();
  escape();
  if (o_aktual->id==300)
     for(i=0;i<MOBS_INV;i++) moblist[m].inv[i]=f_get_value(0,i+10);
  close_current();
  release_list(ls_sorts);
  }

static char *aff_list[]=
  {
  "INVIS",
  "OKO ZA OKO",
  "NASTAV TV��",
  "ENERGY DRAIN",
  "MANA SHIELD (*)",
  "SANCTUARY SHIELD (1/2 zasahu)",
  "HIGH SANCTUARY (omezit zasah)",
  "BLIND (slepota)",
  "REGENERACE (*)",
  "OCHRANA P�ED ZIMOU (*)",
  "OCHRANA P�ED ��REM (*)",
  "KNOCK BACK",
  "< volno >",
  "< volno >",
  "< volno >",
  "Vyhrazeno pro d�mona"
  };

void spell_affections()
  {
  int m,i;
  CTL3D b1;


  m=f_get_value(0,700);
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,50,400,390,"Affections");
  for(i=0;i<16;i++)
     {
     define(i+10,10,20+i*12,250,10,0,check_box,aff_list[i]);c_default((m & (1<<i))!=0);
     }
  define(-1,10,230,1,1,0,label,"(*) - u nestvur neimplementov�no");
  define(-1,10,242,1,1,0,label,"Sanctuary bere potvo�e jen 1/2 �ivot� ze z�sahu");
  define(-1,10,254,1,1,0,label,"High Sanc. o�ez�v� z�sah na max 18 �ivot�");
  define(-1,10,266,1,1,0,label,"Energy drain p�id� 1/2 toho co ubere postav�m");
  define(-1,10,278,1,1,0,label,"BLIND - potvora je slepa (senceless)");
  define(-1,10,290,1,1,0,label,"REGENERACE - Ka�d� kolo +HPREG �ivot�");
  define(300,5,20,80,20,1,button,"Ok");property(&b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(310,5,45,80,20,1,button,"Zru�it");property(&b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  redraw_desktop();
  escape();
  if (o_aktual->id==300)
     {
     for(i=0,m=0;i<16;i++) m|=f_get_value(0,i+10)<<i;
     }
  close_current();
  c_set_value(0,700,m);
  }

void mob_sound_call(int mob)
  {
  int i,j;
  TSTR_LIST list;
  CTL3D b1,b2,b3;
  char *c;

  i=mob;
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,50,300,250,"Zvuk potvory");
  concat(c,sample_path,"*.wav");
  list=read_directory(c,DIR_BREIF,_A_NORMAL);
  read_ddl_list_wav(&list);
  define(9,10,20,200,126,0,listbox,list,RGB555(31,31,31),0);
  property(&b3,NULL,NULL,WINCOLOR);c_default(0);on_control_change(mob_test_sound);
  define(10,217,40,19,87,0,scroll_bar_v,0,10,1,SCROLLBARCOL);
  property(&b2,NULL,NULL,WINCOLOR);
  define(11,216,20,21,17,0,scroll_button,-1,0,"\x1e");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(12,216,130,21,17,0,scroll_button,1,10,"\x1f");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  for(j=0;j<MOB_SOUNDS-1;j++)
     {
     int z,y;
     z=moblist[i].sounds[j];
     y=160+15*j;
     define(-1,5,y,1,1,0,label,sound_ev[j]);
     define(j+20,120,y,120,12,0,input_line,13);
        property(&b2,NULL,NULL,WINCOLOR);
        if (z && enemy_sound!=NULL && (z<=str_count(enemy_sound)))
           set_default(enemy_sound[z-1]);else set_default("");
     define(j+40,245,y,20,12,0,button,"<");
        property(&b1,NULL,NULL,WINCOLOR);on_control_change(mob_sound_copy);
     }
  define(90,5,30,100,10,2,check_box,"Loop (WALK!)");c_default((moblist[i].vlajky & 0x40)!=0);
  define(100,5,5,80,20,2,button,"Ok");on_control_change(terminate_gui); property(&b1,NULL,NULL,WINCOLOR);
  define(110,90,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui); property(&b1,NULL,NULL,WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==100)
     {
     char s[20];
     for(j=0;j<MOB_SOUNDS-1;j++)
        {
        get_value(0,20+j,s);
        strupr(s);
        if (s[0]) moblist[i].sounds[j]=add_mob_sound(s)+1;else moblist[i].sounds[j]=0;
        }
     moblist[i].vlajky&=~0x40;
     moblist[i].vlajky|=f_get_value(0,90)<<6;
     }
  close_current();
  }

static void mob_sound()
  {
  int i;

  i=f_get_value(0,190);
  mob_sound_call(i);
  }

static void open_dialog_edit_box()
  {
  int i,j,k;
  CTL3D b1,b2,b3;

  i=f_get_value(0,190);
  if (!f_get_value(0,k=o_aktual->id))
     {
     moblist[i].dialog=-1;
     return;
     }
  j=moblist[i].dialog;
  memcpy(&b1,def_border(1,0),sizeof(CTL3D));
  memcpy(&b2,def_border(5,WINCOLOR),sizeof(CTL3D));
  memcpy(&b3,def_border(6,WINCOLOR),sizeof(CTL3D));
  default_font=vga_font;
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  def_dialoge(100,250,250,180,"P�i�a� dialog");
  define(9,10,20,200,126,0,listbox,dlg_names,RGB555(31,31,31),0);
  property(&b3,NULL,NULL,WINCOLOR);c_default(pgf2name(j));
  define(10,217,40,19,87,0,scroll_bar_v,0,10,1,SCROLLBARCOL);
  property(&b2,NULL,NULL,WINCOLOR);
  define(11,216,20,21,17,0,scroll_button,-1,0,"\x1e");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(12,216,130,21,17,0,scroll_button,1,10,"\x1f");
  property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
  define(100,5,5,80,20,2,button,"Ok");on_control_change(terminate_gui); property(&b1,NULL,NULL,WINCOLOR);
  define(110,90,5,80,20,2,button,"Zru�it");on_control_change(terminate_gui); property(&b1,NULL,NULL,WINCOLOR);
  redraw_window();
  escape();
  if (o_aktual->id==100)
     {
     moblist[i].dialog=dlg_pgfs[f_get_value(0,9)];
     }
  close_current();
  c_set_value(0,k,moblist[i].dialog>=0);
  }

/*static void ask_dialog(EVENT_MSG *msg,OBJREC *obj)
  {
  if (msg->msg==E_MOUSE)
     {
     MS_EVENT *ms;

     ms=get_mouse(msg);
     if ((ms->event_type & 0xA && *(char *)obj->data))
        open_dialog_edit_box();
     }
  }*/

void edit_mob(int i)
  {
  int y,z;
  CTL3D *b1;

  if (!mglob.local_monsters && check_data_password()==0) return;
  i;
  if (!moblist[i].anim_counts[0]) moblist[i].anim_counts[0]=1;
  def_dialoge(50,20,550,415,"Definice potvory");
  define(200,310,35,290,1,0,pcx_view);set_default("_");
  define(205,310,35,290,370,0,symetry);c_default(moblist[i].adjusting[0]);
  define(207,5,20,120,20,1,input_line,20);set_default("");
  define(-1,190,20,100,12,0,label,"Jm�no potvory");
  define(10,300,20,200,12,0,input_line,31);set_default(moblist[i].name);property(def_border(2,WINCOLOR),NULL,NULL,WINCOLOR);
  z=0;b1=def_border(5,WINCOLOR);
  for(y=20;y<380;)
     {
     if (nvlast[z][0])
	{
     	define(-1,5,y,140,12,0,label,nvlast[z]);
     	define(z+20,120,y,60,12,0,input_line,8,-32767,32767,"%6d");set_default(strs(moblist[i].vlastnosti[z]));on_control_exit(test_int);
     	property(b1,NULL,NULL,WINCOLOR);
	y+=15;
	}
     z+=1;
     if (nvlast[z][0]=='*') break;
     }
  y+=15;
  define(-1,5,y,140,12,0,label,"�ivel:");
  define(102,120,y,60,12,0,str_line,zivly);c_default(moblist[i].vlastnosti[VLS_MGZIVEL]);on_enter(string_list_sup);property(b1,NULL,NULL,WINCOLOR);y+=15;
  define(-1,5,y,140,12,0,label,"Jm�no grafiky");
  define(100,120,y,60,12,0,input_line,6);set_default(moblist[i].mobs_name);property(b1,NULL,NULL,WINCOLOR);on_control_exit(check_mobs_name);y+=15;
  define(-1,5,y,140,12,0,label,"Rychlost");
  define(101,130,y,30,12,0,input_line,3,0,128,"%3d");set_default(strs(moblist[i].speed));on_control_exit(test_int);property(b1,NULL,NULL,WINCOLOR);y+=15;
  define(-1,5,y,140,12,0,label,"�ance �t�ku");
  define(103,130,y,30,12,0,input_line,3,0,255,"%3d");set_default(strs(moblist[i].flee_num));on_control_exit(test_int);property(b1,NULL,NULL,WINCOLOR);y+=15;
  define(-1,5,y,140,12,0,label,"Kouzlo");
  define(104,130,y,30,12,0,input_line,3,0,255,"%3d");set_default(strs(moblist[i].casting));on_control_exit(test_int);property(b1,NULL,NULL,WINCOLOR);y+=15;
  define(-1,5,y,140,12,0,label,"Specproc");
  define(105,120,y,60,12,0,str_line,mob_procs);c_default(moblist[i].specproc);on_enter(string_list_sup);property(b1,NULL,NULL,WINCOLOR);y+=15;
  y=260;define(-1,220,248,140,12,0,label,"Po�et anim.");
  for(z=0;z<6;z++)
     {
     define(-1,190,y,50,12,0,label,smery_anim[z]);
     define(z+120,260,y,20,12,0,input_line,2,!z*2,15,"%2d");set_default(strs(moblist[i].anim_counts[z]));on_control_exit(test_int);
     property(b1,NULL,NULL,WINCOLOR);
     y+=15;
     }
  define(127,285,320,20,12,0,input_line,2,1,15,"%2d");set_default(strs(moblist[i].hit_pos));on_control_exit(test_int);
     property(b1,NULL,NULL,WINCOLOR);
  define(430,190,350,50,10,0,check_box,"Chod�");
  define(440,190,362,50,10,0,check_box,"�to��");
  define(450,190,374,50,10,0,check_box,"Sly��");
  define(460,250,350,100,10,0,check_box,"Jeden na pol��ku");
  define(470,250,362,60,10,0,check_box,"Str��ce");
  define(480,250,374,40,10,0,check_box,"Sebere");
  define(490,250,386,40,10,0,check_box,"Sb�r�");
  define(500,190,386,50,10,0,check_box,"St��l�");
  define(510,20,438,50,10,0,check_box,"Dialog.");c_default(moblist[i].dialog>=0);on_control_change(open_dialog_edit_box);
  define(-1,190,50,50,12,0,label,"Dohled");
  define(-1,190,65,50,12,0,label,"Dosah");
  define(-1,190,80,50,12,0,label,"Expy:");
  define(-1,190,95,50,12,0,label,"Bonus:");
  define(-1,190,110,50,12,0,label,"Pen�ze:");
  define(160,260,50,30,12,0,input_line,3,0,255,"%3d");set_default(strs(moblist[i].dohled));on_control_exit(test_int);property(b1,NULL,NULL,WINCOLOR);
  define(170,260,65,30,12,0,input_line,3,0,255,"%3d");set_default(strs(moblist[i].dosah));on_control_exit(test_int);property(b1,NULL,NULL,WINCOLOR);
  define(175,250,80,55,12,0,input_line,6,0,999999,"%6d");set_default(strs(moblist[i].experience));on_control_exit(test_int);property(b1,NULL,NULL,WINCOLOR);
  define(176,250,95,55,12,0,input_line,6,0,65535,"%6d");set_default(strs(moblist[i].bonus));on_control_exit(test_int);property(b1,NULL,NULL,WINCOLOR);
  define(177,255,110,50,12,0,input_line,6,0,65535,"%5d");set_default(strs(moblist[i].money));on_control_exit(test_int);property(b1,NULL,NULL,WINCOLOR);
  define(180,0,0,0,0,0,value_store,4);c_default(0);//cislo_animace
  define(185,0,0,0,0,0,value_store,4);c_default(0);//cislo_pozice
  define(190,0,0,0,0,0,value_store,4);c_default(i);//cislo_i
  b1=def_border(1,0);
  define(210,190,144,80,20,0,button,"Dal�� >>");property(b1,NULL,NULL,WINCOLOR);on_control_change(mob_next);
  define(220,190,166,80,20,0,button,"Oto�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(mob_turn);
  define(230,190,188,80,20,0,button,"Test");property(b1,NULL,NULL,WINCOLOR);on_control_change(play_mob);
  define(240,84,2,80,20,2,button,"Zru�it");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(250,2,2,80,20,2,button,"Ok");property(b1,NULL,NULL,WINCOLOR);on_control_change(terminate_gui);
  define(260,166,2,80,20,2,button,"Vymazat");property(b1,NULL,NULL,WINCOLOR);on_control_change(mob_ask_delete);
  define(270,2,24,80,20,2,button,"Zvuky");property(b1,NULL,NULL,WINCOLOR);on_control_change(mob_sound);
  define(280,84,24,80,20,2,button,"Invent��");property(b1,NULL,NULL,WINCOLOR);on_control_change(mob_inv);
  define(290,166,24,80,20,2,button,"Aff");property(b1,NULL,NULL,WINCOLOR);on_control_change(spell_affections);
  define(-1,190,220,1,1,0,label,"Pals:");
  define(520,230,220,30,12,0,input_line,3,-127,127,"%3d");set_default(strs(moblist[i].paletts_count));on_control_exit(test_int);property(b1,NULL,NULL,WINCOLOR);
  for(y=0;y<6*16;y++) {define(300+y,0,0,0,0,0,value_store,4);c_default(moblist[i].adjusting[y]);}
  define(630,0,0,0,0,0,value_store,1);c_default(0);//IN_BATTLE;
  define(640,190,400,50,10,0,check_box,"Pr�choz�");
  define(650,190,412,50,10,0,check_box,"C�t�");
  define(660,190,424,100,10,0,check_box,"Astral (wind)");
  define(670,300,400,100,10,0,check_box,"Reload mob");
  define(680,300,412,100,10,0,check_box,"Kouzl�");
  define(700,0,0,0,0,0,value_store,4);c_default(moblist[i].vlastnosti[VLS_KOUZLA]);
  movesize_win(waktual,0,0,DxGetResX(),DxGetResY());
  check_mobs_name();
  fill_bit_fields(0,430,moblist[i].stay_strategy,8);
  fill_bit_fields(0,630,moblist[i].vlajky,6);
  redraw_desktop();
  escape();
  if (o_aktual->id==250)
     {
     TMOB *p;
     p=&moblist[i];
     get_value(0,10,p->name);
     for(z=0;z<24;z++)
	if (nvlast[z][0]) p->vlastnosti[z]=vals(20+z);else p->vlastnosti[z]=0;
     get_value(0,100,p->mobs_name);
     p->speed=vals(101);
     p->vlastnosti[VLS_MGZIVEL]=f_get_value(0,102);
     p->flee_num=vals(103);
     p->casting=vals(104);
     p->specproc=f_get_value(0,105);
     for(z=0;z<6;z++) p->anim_counts[z]=vals(120+z);
     p->hit_pos=vals(127);
     p->stay_strategy=get_bit_fields(0,430,8);
     p->vlajky=get_bit_fields(0,630,6) | (p->vlajky & 0x40);
     p->dohled=vals(160);
     p->dosah=vals(170);
     p->experience=vals(175);
     p->bonus=vals(176);
     p->money=vals(177);
     p->vlastnosti[VLS_KOUZLA]=f_get_value(0,700);
     for(z=0;z<6*16;z++) p->adjusting[z]=f_get_value(0,300+z);
     if (p->name[0]=='\0')
        {
        msg_box("Doporu�en�",' ',"Bylo by dobr� potvoru nazvat, kvuli snadn� manipulaci","Ok",NULL);
        strcpy(p->name,p->mobs_name);
        }
     p->lives=p->vlastnosti[VLS_MAXHIT];
     p->cislo_vzoru=i;
     p->locx=128;
     p->headx=128;
     p->locy=128;
     p->heady=128;
     for(y=0;y<6;y++)
        for(z=0;z<16;z++)
           if (moblist[i].anim_counts[y]==0) moblist[i].adjusting[z+y*16]=moblist[i].adjusting[z];
     p->paletts_count=vals(520);
     }
  z=o_aktual->id;
  close_current();
  if (z==260)
     {
     int j;
     char redraw=0;
     for(j=0;j<maplen;j++) if ((mob_map[j] & 0x3fff)==i)
        {
        redraw=1;
        jdi_na_sektor(j);
        if (msg_box("Potvora je v map�. Vymazat?",'\0x2',"Chce� vymazat tuto potvoru a v�echny dal��?","Ano","Ne",NULL)==2) return;
        break;
        }
     for(j=0;j<maplen;j++) if ((mob_map[j] & 0x3fff)==i) mob_map[j]=-1;
     moblist[i].cislo_vzoru=-1;
     if (redraw) redraw_desktop();
     }
  check_unused_sounds();
  }

void edit_mob_event()
  {
  int i;
  i=f_get_value(0,29);
  if (enemy_list[i]==NULL) return;
  sscanf(enemy_list[i],"%d",&i);
  if (i>=0 && i<TOTAL_MOBS) edit_mob(i);
  create_enemy_list();
  send_message(E_GUI,29,E_CONTROL,1,enemy_list);
  }

void umistit_moba()
  {
  int i,j;
  i=f_get_value(0,29);
  if (enemy_list[i]==NULL) return;
  sscanf(enemy_list[i],"%d",&i);
  for(j=0;j<maplen;j++) if (minfo[j].flags & 1)
        {
        if (mob_map[j]!=-1 && (mob_map[j] & 0x3fff)==i) mob_map[j]+=16384;else mob_map[j]=i;
        }
  redraw_desktop();
  }

void vymazat_moba()
  {
  int j;
  for(j=0;j<maplen;j++) if (minfo[j].flags & 1) mob_map[j]=-1;
  redraw_desktop();
  }


void enemy_window()
  {
  memcpy(f_default,flat_color(0x0000),sizeof(charcolors));
  tool_sel=30;
  create_map_win(3,3,SCR_WIDTH_X-200,SCR_WIDTH_Y-10);
  if (find_window(enemy_win)==NULL)
     {
     int i;
     CTL3D *b1;

     create_enemy_list();
     default_font=vga_font;
     for (i=1;i<maplen;i++) if (minfo[i].flags & 1) break;
     if (i==maplen) i=0;
     enemy_win=def_window(200,280,"Potvory");
     waktual->minsizx=140;
     waktual->minsizy=150;
     define(-1,3,1,10,10,2,resizer);
     define(29,5,20,170,190,0,listbox,enemy_list,RGB555(31,31,0),4);c_default(0);
        o_end->autoresizex=1;
        o_end->autoresizey=1;
     b1=def_border(3,WINCOLOR);
     define(30,5,38,15,156,1,scroll_bar_v,0,10,1,SCROLLBARCOL);
        property(b1,NULL,NULL,WINCOLOR);
        o_end->autoresizey=1;
     define(31,3,20,19,15,1,scroll_button,-1,0,"\x1e");
        property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
     define(32,3,68,19,15,2,scroll_button,1,10,"\x1f");
        property(NULL,icones,NULL,WINCOLOR);on_control_change(scroll_support);
     define(40,5,5,60,20,3,button,"Vymazat");property(b1,NULL,NULL,WINCOLOR);on_control_change(vymazat_moba);
     define(50,5,30,60,20,3,button,"Um�stit");property(b1,NULL,NULL,WINCOLOR);on_control_change(umistit_moba);
     define(60,70,5,60,20,3,button,"Novou");property(b1,NULL,NULL,WINCOLOR);on_control_change(new_mob);
     define(70,70,30,60,20,3,button,"Opravit");property(b1,NULL,NULL,WINCOLOR);on_control_change(edit_mob_event);
     movesize_win(waktual,SCR_WIDTH_X,0,170,SCR_WIDTH_Y-20);
     redraw_window();
     }
  else
     select_window(enemy_win);
  }

void select_enemy(int at_sector)
  {
  int i,num,pos;

  num=mob_map[at_sector];
  if (num==-1) return;
  num&=0x3fff;
  pos=0;
  for(i=0;i<num;i++) if (moblist[i].cislo_vzoru!=-1) pos++;
  if (moblist[i].cislo_vzoru==-1) return;
  c_set_value(enemy_win,29,pos);
  select_window(enemy_win);
  redraw_window();
  send_message(E_GUI,29,E_CONTROL,2);
  select_window(map_win);
  }

void save_mobs()
  {
  FILE *f;int i;

  f=fopen(MOB_FILE,"wb");
  if (f==NULL) return;
  i=MOB_VER;
  fwrite(&i,1,sizeof(i),f);
  i=sizeof(TMOB);
  fwrite(&i,1,sizeof(i),f);
  for(i=0;i<TOTAL_MOBS;i++)
     if (moblist[i].cislo_vzoru!=-1) fwrite(&moblist[i],1,sizeof(TMOB),f);
  fclose(f);
  }

void load_mobs()
  {
  FILE *f;int i,s;
  TMOB *x;

  memset(moblist,0xff,sizeof(moblist));
  f=fopen(MOB_FILE,"rb");
  if (f==NULL) return;
  fread(&i,1,sizeof(i),f);
  fread(&s,1,sizeof(s),f);
  x=(TMOB *)getmem(s);
  if (i==MOB_VER)
     while (fread(x,1,s,f)) moblist[x->cislo_vzoru]=*x;
  free(x);
  fclose(f);
  }

void *save_mobs_to_map(long *size)
  {
  int i,cnt;
  TMOB *m,*c;

  for(i=0,cnt=0;i<TOTAL_MOBS;i++) if (moblist[i].cislo_vzoru!=-1) cnt++;
  if (!cnt) return NULL;
  c=m=NewArr(TMOB,cnt);
  for(i=0,c=m;i<TOTAL_MOBS;i++,c++) if (moblist[i].cislo_vzoru!=-1) memcpy(c,moblist+i,sizeof(TMOB));
  *size=cnt*sizeof(TMOB);
  return m;
  }

void load_mobs_to_map(void *p,long size)
  {
  TMOB *m;
  int i,cnt;

  memset(moblist,0xff,sizeof(moblist));
  cnt=size/sizeof(TMOB);
  for(i=0,m=p;i<cnt;i++,m++) moblist[m->cislo_vzoru]=*m;
  }

void *save_mob_map(long *size)
  {
  int i,c;
  short *data,*p;

  for(i=0,c=0;i<maplen;i++)
     if (mob_map[i]!=-1) c++;
  if (!c) return NULL;
  if (c>255) msg_box("Omezen�",'\x1',"Nelze m�t v jedn� map� v�c potvor ne� 255. MAPEDIT n�kter� nenahraje.","Ok",NULL);
  p=data=getmem(*size=c*4);
  for(i=0;i<maplen;i++)
     if (mob_map[i]!=-1)
        {
        *p++=i;
        *p++=mob_map[i];
        }
  return data;
  }


void load_mob_map(void *data,int size)
  {
  int i,end;
  short *p;

  end=size/4;
  p=data;
  for(i=0;i<end;i++,p+=2) mob_map[p[0]]=p[1];
  }

static void check_correct_sounds()
  {
  int c,i,j,z;
  char err;
  c=str_count(enemy_sound);
  for(i=0;i<TOTAL_MOBS;i++)
     {
     err=0;
     if (moblist[i].cislo_vzoru!=-1)
        for(j=0;j<MOB_SOUNDS;j++)
           {
           z=moblist[i].sounds[j];
           if (z && (z>c || enemy_sound[z-1]==NULL))
              {
              moblist[i].sounds[j]=0;
              err=1;
              }
           }
     if (err)
        {
        char text[200];
        sprintf(text,"Nestv�ra \"%s\" m� �patnou referenci zvuku! Polo�ka bude vynulov�na!",moblist[i].name);
        if (msg_box("Chybn� reference",'\x1',text,"Pokra�ovat","Opravit",NULL)==2) mob_sound_call(i);
        }
     }
  }

void save_sound_map()
  {
  FILE *f;
  int i,c;

  if (enemy_sound==NULL)
     {
     remove(MOB_SOUND);
     return;
     }
  check_correct_sounds();
  f=fopen(MOB_SOUND,"wb");
  c=str_count(enemy_sound);
  while (c>0 && enemy_sound[c-1]==NULL) c--;
  fwrite(&c,1,4,f);
  for(i=0;i<c;i++)
     {
     if (enemy_sound[i]==NULL) fwrite(&enemy_sound[i],1,1,f);
     else fwrite(enemy_sound[i],1,strlen(enemy_sound[i])+1,f);
     }
  fclose(f);
  }

void load_sound_dat(void *p,long siz)
  {
  int i;
  char *c,*s;

  c=s=p;
  i=0;
  while (c-s<siz)
     {
     if (c[0]) str_replace(&enemy_sound,i,c);
     c=strchr(c,0);
     if (c==NULL)
        {
        msg_box("Load Error",'\x1',"Chyba nastala p�i na��t�n� souboru SOUND.DAT: Nekonzistetn� �daje. N�kter� zvuky nebudou p�i�azeny!","Ok",NULL);
        break;
        }
     c++;
     i++;
     }
  }
void load_sound_map()
  {
  FILE *f;
  long siz;
  char *c,*s;

  f=fopen(MOB_SOUND,"rb");
  if (f==NULL) return;
  if (enemy_sound!=NULL) release_list(enemy_sound);
  enemy_sound=create_list(4);
  fseek(f,0,SEEK_END);
  siz=ftell(f)-4;fseek(f,4,SEEK_SET);
  s=c=(char *)getmem(siz);
  memset(s,0,siz);
  fread(s,1,siz,f);
  fclose(f);
  load_sound_dat(s,siz);
  free(s);
  check_correct_sounds();
  }

int save_sound_dat(FILE *f,int id)
  {
  int i,cnt;
  TSTR_LIST ls;

  cnt=str_count(enemy_sound);
  ls=create_list(cnt+1);
  for(i=0;i<cnt;i++) if (enemy_sound[i]!=NULL)str_replace(&ls,i+1,enemy_sound[i]);
  i=save_scr_list(f,ls,id);
  release_list(ls);
  return i;
  }

void remove_mobs_from_sector(int sector)
  {
  mob_map[sector]=-1;
  }

