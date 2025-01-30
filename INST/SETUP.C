#include <process.h>
#include <libs/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <mem.h>
#include <malloc.h>
#include <libs/pcx.h>
#include <libs/memman.h>
#include <libs/event.h>
#include <libs/bmouse.h>
#include <libs/gui.h>
#include <io.h>
#include <conio.h>
#include <graph.h>
#include <libs/basicobj.h>
#include <libs/zvuk.h>
#include <libs/bgraph.h>
#include <strlists.h>
#include <libs/strlite.h>
#include <direct.h>
#include <dos.h>
#include <vesa.h>
#include <doserr.h>
#include <libs/zvuk.h>
#include <libs/inicfg.h>
#include "setup.h"
#include "setvideo.h"
#include "setuplib.h"
#include "setupcpy.h"

#define TEST_MUSIC "\\SNDTEST.MUS"
#define SETUP_BATCH "SETUP.BAT"
#define _SOURCE_ copy_source_path
#define ER_NAME "c:\\skeldal.!!!"
#define VEL_RAMECEK 8

#define VMODE "VMODE"
#define SOUND_DEVICE "SOUND_DEVICE"

#define SETUP_NAME "SETUP.EXE"

word *ramecky[VEL_RAMECEK];

#define SKELDAL "SKELDAL"
#define SKELDAL_INI SKELDAL".INI"
#define CONCAT(t,s1,s2) strcat(strcpy(t=alloca(strlen(s1)+strlen(s2)+1),s1),s2)
void *bbutt;

char *copy_source_path;
char *test_sound_name;
char *program_data_path=NULL;
char setup_mode=0;

char *pgm_name;

TSTR_LIST setup_ini;

typedef struct tsound
  {
  int device,port,irq,dma;
  }TSOUND;


FILE *ini;
word *vga_font;
word *icones;
word icone_color[7]={0x2108,0x7fff,0x000f,0x4210,0x6f7b};
//static char target_path[256]="E:\HRY\SKELDAL\";
char *source_path;
char target_path[2049];
static TSOUND sound_info={0,0,0,0};
static int vmode=0;
static char mixer_running=0;
static char back=0;
static char install_type;
static long install_sizes[3];
static long maxcopy=1;
TSTR_LIST handbook=NULL;
TSTR_LIST dirlist=NULL;
TSTR_LIST disklist=NULL;
int win_handbook=-1;
char rescue_mode=0;
char *inifile=NULL;

#pragma off (stack_checking)

static char home_path(char *source_path)
  {
  unsigned x,z;
  _dos_setdrive(source_path[0]-'@',&x);
  _dos_getdrive(&z);
  if (z!=source_path[0]-'@') return 1;
  chdir("\\");
  chdir(source_path);
  return 0;
  }
char error_device(word a,char b,char c)
  {
  a,b,c;

  errno=-1;
  return _ERR_FAIL;
  }

int create_er_file()
  {
  FILE *f;
  if (!access(ER_NAME,F_OK)) return 0;

  f=fopen(ER_NAME,"w");
  fprintf(f,"7925536\n");
  fprintf(f,"Toto je zachranny soubor pro instalacni program ke hre BRANY SKELDALU. Pokud se posledni instalace nezdarila, nemazte tento soubor!");
  fclose(f);
  return 1;
  }

void remove_er_file()
  {
  FILE *f;

  f=fopen(ER_NAME,"r");
  if (f!=NULL)
     {
     long l=0;
     fscanf(f,"%d",&l);
     fclose(f);
     if (l==7925536) remove(ER_NAME);
     }
  }


static conv_ramecek(word *ram)
  {
  int i;
  word xs,ys;
  char *ptr;
  word *targ;

  xs=ram[0];
  ys=ram[1]/VEL_RAMECEK;
  ptr=(char *)(ram+3+256);
  for(i=0;i<VEL_RAMECEK;i++)
     {
     targ=ramecky[i]=(word *)getmem(xs*ys+6+512);
     targ[0]=xs;
     targ[1]=ys;
     targ[2]=ram[2];
     memcpy(targ+3,ram+3,512);
     memcpy(targ+3+256,ptr,xs*ys);
     ptr+=xs*ys;
     }
  }

EVENT_PROC(music_mixer)
  {
  GET_USER_PTR();
  WHEN_MSG(E_TIMER)
     {
     mix_back_sound(0);
     }
  return;
  }

static void cpy_error_proc(int event,char *name)
  {
  char *hlaska;
  char *text;

  switch(event)
     {
     case CPERR_OPEN:hlaska="Nemohu otev��t soubor :";break;
     case CPERR_READ:hlaska="Nastala chyba p�i �ten� z CD. Vyjm�te CD, o�ist�te jej a op�t ho vlo�te do mechaniky";break;
     case CPERR_WRITE:hlaska="Nastala chyba p�i zapisu na disk. Mo�n� u� je pln�. Soubor:";break;
     default:hlaska="Nastala nezn�ma chyba. ";
     }
  text=alloca(strlen(hlaska)+strlen(name)+10);
  strcpy(text,hlaska);
  if (name!=NULL)strcat(text,name);
  if (msg_box("Chyba!",'\x1',text,"Opakuj","Konec",NULL)==2)
     {
     shutdown();
     exit(1);
     }
  do_events();
  }

static void stop_copy()
  {
  if (msg_box("Konec instalace?",'\x2',"Hra je�t� nen� kompletn� nainstalov�na. Opravdu chce� ukon�it instal�tora?","Ano","Ne",NULL)==1)
     {
     close_ini();
     shutdown();
     exit(1);
     }
  }

static void stop_setup()
  {
  if (msg_box("Konec?",'\x2',"Tato volba ukon�� SETUP bez ulo�en� zm�n. Opravdu chce� ukon�it program?","Ano","Ne",NULL)==1)
     {
     shutdown();
     home_path(target_path);
     exit(1);
     }
  }


static void display_progress(void)
  {
  def_dialoge(312,380,310,75,"Kop�ruj� se soubory...",3);
  define(10,10,27,290,15,0,done_bar,100);
  property(bbutt,NULL,flat_color(0x1e0),WINCOLOR);
  define(30,5,5,80,15,2,button,"P�eru�it");on_control_change(stop_copy);
   property(def_border(1,0),&font6x9,flat_color(0),BUTTONCOLOR);
  define(20,5,10,200,12,3,input_line,2048);set_default("");
   property(NULL,&font6x9,flat_color(0),WINCOLOR);
  c_default(0);
  redraw_window();
  exit_wait=0;
  }

static void cpy_progress_proc(long l)
  {
  c_set_value(0,10,(int)((float)l/(float)maxcopy*100));
  do_events();
  }

static void init_setup(char automode)
  {
  int c;
  char *pc1,*pc2;
  conv_ramecek(&ramecek);
  source_path=getcwd(NULL,PATH_MAX+1);
  c=strlen(source_path)-1;
  if (source_path[c]=='\\') source_path[c]=0;
  if (copy_source_path==NULL) copy_source_path=source_path;
  if (program_data_path==NULL) program_data_path=source_path;
  if (automode) vmode=initgr_auto();else if (vmode) initgr_spec(vmode);else initgr_low();
  init_events();
  install_gui();
  CONCAT(pc1,program_data_path,"\\install.pcx");
  CONCAT(pc2,program_data_path,"\\install.hi");
  if (open_pcx(pc1,A_8BIT,&gui_background))
     if (access(pc2,F_OK)==0)
       gui_background=load_file("setup.hi");
       else gui_background=NULL;
  zobraz_mysku();
  update_mysky();
  redraw_desktop();do_events();
  msg_box_font=&boldcz;
  msg_icn_font=&ikones;
  bbutt=New(CTL3D);
  memcpy(bbutt,def_border(1,0),sizeof(CTL3D));
  memcpy(f_default,flat_color(0),sizeof(f_default));
  vga_font=default_font=&boldcz;
  icones=&ikones;
  install_dos_error(error_device,NewArr(char,2048)+2048);
  cpy_error=cpy_error_proc;
  cpy_progress=cpy_progress_proc;
  install_cpy();
  send_message(E_ADD,E_TIMER,music_mixer);
  }

static void shutdown()
  {
  if (mixer_running) stop_mixing();
  fcloseall();
  home_path(source_path);
  remove_er_file();
  clean_up();
  donegr();
  }


TSTR_LIST read_text(char *filename)
  {
  TSTR_LIST ls;
  char text[81];
  FILE *f;

  ls=create_list(10);
  f=fopen(filename,"r");
  if (f==NULL) return NULL;
  while (!feof(f))
     {
     char *c;
     fgets(text,80,f);
     c=strchr(text,'\n');if (c!=NULL) *c=0;
     if (text[0]==0)
        {
        text[0]=32;
        text[1]=0;
        }
     str_add(&ls,text);
     }
  fclose(f);
  return ls;
  }

static void open_handbook(char *file)
  {
  if (find_window(win_handbook)!=NULL) close_window(find_window(win_handbook));
  if (handbook!=NULL) release_list(handbook);
  handbook=read_text(file);
  win_handbook=def_dialoge(54,54,450,350,"P��ru�ka",2);
  def_listbox(9,5,25,420,290,handbook,0,WINCOLOR);property(def_border(0,WINCOLOR),&font6x9,NULL,WINCOLOR);
  define(20,5,5,60,15,2,button2,"Zav��t");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(close_current);
  redraw_window();
  }

static void open_help()
  {
  open_handbook("SETUP.HLP");
  }


static set_full_target_path(int i)
  {
  if (target_path[0]!=0 && target_path[strlen(target_path)-1]=='\\')
     strcat(target_path,SKELDAL);
  else
     strcat(target_path,"\\"SKELDAL);
  if (i==0) set_default(target_path); else set_value(0,i,target_path);
  *(strrchr(target_path,'\\'))=0;
  }

static void read_dirlist()
  {
  int i;
  if (dirlist!=NULL) release_list(dirlist);
  dirlist=read_directory("*.*",DIR_NAMES,_A_SUBDIR);
  if (dirlist==NULL)
     {
     dirlist=create_list(1);
     str_add(&dirlist,"<nic>");
     }
  for(i=0;i<str_count(dirlist);i++)
     {
     if (dirlist[i]!=NULL && !strcmp(dirlist[i],".")) str_replace(&dirlist,i,NULL);
     else
     if (dirlist[i]!=NULL && !strcmp(dirlist[i],".."))
        {
        str_insline(&dirlist,0,"<- zp�t");
        str_replace(&dirlist,i,NULL);
        i++;
        }
     }
  str_delfreelines(&dirlist);
  }

static read_disk_list()
  {
  unsigned i,j,c,d;
  if (disklist!=NULL) release_list(disklist);
  disklist=create_list(26);
  _dos_getdrive(&d);
  j=256;
  for(i=3;i<j;i++)
     {
     _dos_setdrive(i,&j);
     _dos_getdrive(&c);
     if (c==i)
        {
        char c[20];

        sprintf(c,"[-%c-]",i+'@');
        str_add(&disklist,c);
        }
     }
  _dos_setdrive(d,&j);
  }

static void update_list()
  {
  char *c;
  getcwd(&target_path,2047);
  c=target_path+strlen(target_path);
  if (c!=target_path && *(c-1)=='\\') *(c-1)=0;
  set_full_target_path(10);
  read_dirlist();
  send_message(E_GUI,59,E_CONTROL,1,dirlist);
  c_set_value(0,59,0);
  send_message(E_GUI,59,E_CONTROL,2);
  redraw_window();
  redraw_window();
  }

static void select_dir()
  {
  int i=f_get_value(0,59);
  if (dirlist[i]==NULL) return;
  if (!strcmp(dirlist[i],"<- zp�t")) chdir("..");else chdir(dirlist[i]);
  update_list();
  }



static get_script(char *source,char *target)
  {
  int i;
  i=read_script("install.inf",source,target);
  if (i)
     {
     shutdown();
     if (i==1) puts("Nemohu na��st INSTALL.INF");
     if (i==2) puts("Chyba kompilace scriptu INSTALL.INF");
     abort();
     }
  }

void recalculate()
  {
  char s[20];

  sprintf(s,"%d Kb",get_disk_free(0)/1024);
  set_value(0,100,s);
  }

static void change_disk()
  {
  int i=f_get_value(0,69);
  unsigned d,m;
  if (disklist[i]==NULL) return;
  d=disklist[i][2]-'@';
  _dos_setdrive(d,&m);
  update_list();
  recalculate();
  }


static load_window()
  {
  int i;
  read_dirlist();
  read_disk_list();
  def_dialoge(200,100,340,300,PRG_HEADER,3);
  define(-1,5,20,1,1,0,label,"Jm�no slo�ky");
  define(10,5,35,330,10,0,input_line,2048);property(def_border(5,WINCOLOR),&font6x9,NULL,0x7fff);
  set_full_target_path(0);
  for(i=0;i<str_count(disklist);i++) if (target_path[0]==disklist[i][2]) break;
  default_font=&font6x9;
  define(150,70,5,60,20,2,button,"Ok");on_control_change(terminate_gui);property(bbutt,&font6x9,NULL,BUTTONCOLOR);
  define(160,5,5,60,20,2,button,"Storno");on_control_change(terminate_gui);property(bbutt,&font6x9,NULL,BUTTONCOLOR);
  default_font=vga_font;
  def_listbox(59,70,60,100,200,dirlist,0,0x03ff);c_default(0);on_control_change(select_dir);
  def_listbox(69,5,60,40,200,disklist,0,0x03ff);on_control_change(change_disk);
  c_default(i);
  default_font=&font6x9;
  define(-1,215,135,110,80,0,win_label,"");property(def_border(5,0x631f),NULL,NULL,0x631f);
  define(-1,220,140,1,1,0,label,"Voln� na HD:");
  define(100,240,150,80,10,0,view_line,20);set_default("");property(NULL,&font6x9,NULL,0x631f);
  default_font=vga_font;
  redraw_window();
  recalculate();
  }

static char *gr_mody[]=
  {
  "VGA 256 barev",
  "SVGA 256 barev",
  "HICOLOR"
  };

static void autodetect()
  {
  SVGAinfo si;
  char s[80];

  default_font=&font6x9;
  def_dialoge(12,12,324,140,"Autodetekce",2);
  define(-1,5,22,1,1,0,label,"Grafick� karta:");
  define(-1,5,34,1,1,0,label,"Typ SVGA:");
  define(-1,5,46,1,1,0,label,"Rozli�en�:");
  define(-1,5,62,1,1,0,label,"Zvukov� karta:");
  define(-1,5,74,1,1,0,label,"Parametry:");
  define(-1,5,90,1,1,0,label,"CD-ROM:");
  define(-1,5,110,1,1,0,label,"My�:");
  define(10,5,22,120,10,1,view_line,256);set_default("");
  define(20,5,34,160,10,1,view_line,256);set_default("");
  define(30,5,46,150,10,1,view_line,256);set_default("");
  define(40,5,62,120,10,1,view_line,256);set_default("");
  define(50,5,74,140,10,1,view_line,256);set_default("");
  define(60,5,90,150,10,1,view_line,256);set_default("");
  define(70,5,110,180,10,1,view_line,256);set_default("");
  set_enable(0,6,0);
  set_enable(0,5,0);
  do_events();
  if (getsvgainfo(&si))
     {
     set_value(0,10,"<nen�>");
     set_value(0,20,"<nen�>");
     }
  else
     {
     sprintf(s,"VESA %d.%02d",si.version/256,si.version%256);
     set_value(0,10,s);
     set_value(0,20,si.oemstr);
     }
  set_value(0,30,gr_mody[vmode]);do_events();
  sound_detect(&sound_info.device,&sound_info.port,&sound_info.dma,&sound_info.irq);
  set_value(0,40,device_name(sound_info.device));
  if (sound_info.device==DEV_WSS || sound_info.device==DEV_ULTRA)
     sprintf(s,"Port %X Dma %X",sound_info.port,sound_info.dma);
  else sprintf(s,"Port %X Dma %X Irq %X",sound_info.port,sound_info.dma,sound_info.irq);
  set_value(0,50,s);
  set_value(0,60,get_cdrom());
  set_enable(0,6,1);
  set_enable(0,5,1);
  }

static EVENT_PROC(esc_mode2)
  {
  GET_USER_PTR();
  WHEN_MSG(E_KEYBOARD)
     {
     if (GET_DATA(char)==27)
        {
        donegr();
        if (vmode==0)initgr_low();else initgr_spec(vmode);
        zobraz_mysku();
        redraw_desktop();
        goto_control(0);
        terminate_gui();
        }
     }
  }

static char test_mode()
  {
  char i,j,c;

  if (msg_box("Test grafiky",'\x1',
        "Instal�tor nyn� vyzkou�� zvolen� grafick� re�im. "
        "Pokud obrazovka z�stane �ern�, nebo zobraz� nesmysly, stiskn�te "
        "ESC a instal�tor obnov� p�vodn� re�im.","Start","Zru�it",NULL)==2) return 0;
  send_message(E_ADD,E_KEYBOARD,esc_mode2);
  donegr();
  i=initgr_spec(f_get_value(0,9));
  if (!i)
     {
     zobraz_mysku();
     redraw_desktop();
     }
  if (!i && (j=msg_box("Test grafiky",'\x2',"Vidi� prost�ed� instal�toru spr�vn�?","Ano","Ne",NULL))==1)
     {
     vmode=f_get_value(0,9);
     if (vmode==0) initgr_low();
     zobraz_mysku();
     c=1;
     }
  else
     {
     c_set_value(0,9,vmode);
     if (j!=0)
        {
        donegr();
        if (vmode==0) initgr_low();else initgr_spec(vmode);
        zobraz_mysku();
        redraw_desktop();
        }
     c=0;
     }
  send_message(E_DONE,E_KEYBOARD,esc_mode2);
  return c;
  }

static TSTR_LIST video_ls=NULL;

static void create_video_ls()
  {
  int i;
  if (video_ls==NULL)
     {
     video_ls=create_list(3);
     for(i=0;i<3;i++)  str_add(&video_ls,gr_mody[i]);
     }
  }

static void select_vga()
  {
  default_font=&font6x9;
  def_dialoge(20,300,156,156,"Kvalita grafiky",2);
  define(9,0,20,156,80,0,listbox,video_ls,0x03ff,0);c_default(vmode);
    property(NULL,vga_font,NULL,WINCOLOR);
  define(20,38,5,80,20,3,button,"Zm�nit te�");on_control_change(test_mode);
    property(bbutt,NULL,NULL,BUTTONCOLOR);
  redraw_window();
  }


static void change_dma()
  {
  int i,j;

  i=f_get_value(0,30);
  j=f_get_value(0,35)+3;
  if (o_aktual->id==30)
     {
     c_set_value(0,30,i);
     c_set_value(0,35,i-3);
     }
  else
     {
     c_set_value(0,30,j);
     c_set_value(0,35,j-3);
     }
  }

static void select_mode_win()
  {
  def_dialoge(224,270,192,156,"Instalovat",2);
  default_font=&font6x9;
  define(10,30,40,132,30,0,button,"Automaticky");property(bbutt,NULL,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  define(30,30,80,132,30,0,button,"Podle p��n�");property(bbutt,NULL,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  define(40,50,15,92,20,3,button,"Konec");property(bbutt,NULL,NULL,BUTTONCOLOR);on_control_change(stop_copy);
  redraw_window();
  }

static void select_mode_win_setup()
  {
  def_dialoge(224,270,192,156,"Mo�nosti:",2);
  default_font=&font6x9;
  define(10,30,40,132,30,0,button,"Nastaven�");property(bbutt,NULL,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  define(30,30,80,132,30,0,button,"Odinstalov�n�");property(bbutt,NULL,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  define(40,50,15,92,20,3,button,"Konec");property(bbutt,NULL,NULL,BUTTONCOLOR);on_control_change(stop_setup);
  redraw_window();
  }



static void device_select()
  {
  int i,p;
  i=f_get_value(0,9);
  set_enable(0,40,i!=DEV_ULTRA && i!=DEV_WSS && i!=DEV_NOSOUND && i!=DEV_DAC);
  p=i==DEV_SB16 || i==DEV_ULTRA;
  set_enable(0,35,p);
  if (p==0)
     {
     c_set_value(0,30,1);
     c_set_value(0,35,1-3);
     }
  set_enable(0,20,i!=DEV_NOSOUND);
  set_enable(0,30,i!=DEV_NOSOUND && i!=DEV_DAC);
  set_enable(0,50,i!=DEV_NOSOUND);
  }

static void device_select2()
  {
  int i;

  device_select();
  i=f_get_value(0,9);
  if (i==DEV_DAC)
     {
     char c;
     def_dialoge(270,240,100,100,device_name(i),3);
     define(10,10,25,60,40,0,radio_butts,3,"LPT 1","LPT 2","PC Speaker");c_default(0);
     define(20,10,5,80,20,2,button,"Ok");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
     redraw_window();
     escape();
     c=f_get_value(0,10);
     close_current();
     if (c==0) set_value(0,20,"378");else
        if (c==1)set_value(0,20,"278");
     else set_value(0,20,"042");
     }
  if (i==DEV_SB16 || i==DEV_ULTRA || i==DEV_SBPRO || i==DEV_SB20 || i==DEV_SB10)
		 set_value(0,20,"220");
	if (i==DEV_WSS) set_value(0,20,"530");
  }

static void select_sound();
static void detect_sound()
  {
  if (msg_box("Rozpozn�n� zvukov� karty",'\x2',"Opravdu chce� nechat rozpoznat zvukovou kartu? Na n�kter�ch po��ta��ch m��e automatick� rozpozn�v�n� zp�sobit zatuhnut� po��ta�e.","Ano","Ne",NULL)==2) return;
  if (mixer_running) stop_mixing();
  mixer_running=0;
  set_enable(0,70,mixer_running);
  sound_detect(&sound_info.device,&sound_info.port,&sound_info.dma,&sound_info.irq);
  close_current();
  select_sound();
  }

static int sound_win=-1;

static char sound_scan()
  {
  char buffer[20];
  char dmas[]={0,1,3,5,6,7};
  char irqs[]={2,3,5,7};

  sound_info.device=f_get_value(sound_win,9);
  if (sound_info.device!=DEV_NOSOUND)
     {
     if (sound_info.device==DEV_DAC)
        if (msg_box("Varov�n�!",'\x1',"Vybran� zvukov� za��zen� pou��v� nestandardn� p��stupy a vyu��v� "
        "n�kter�ch skryt�ch trik� syst�mu. Na n�kter�ch po��ta��ch nemus� pracovat spr�vn�. "
        "Toto za��zen� nedok��e pracovat pod WINDOWS95 nebo jin�m OS v�jma DOSu. "
        "Pro zrychlen� pr�ce odinstalujte ze syst�mu jak�koliv EMM manager (EMM386, QEMM, apod.) "
        "Chcete pokra�ovat?","Ano","Ne",NULL)==2) return 1;
     get_value(sound_win,20,buffer);
     if (sscanf(buffer,"%x",&sound_info.port)!=1)
        {
        msg_box("Pozor!",'\x1',"Mus�� vyplnit spr�vn� Port","Ok",NULL);
        return 1;
        }
     }
  sound_info.dma=dmas[f_get_value(sound_win,30)];
  sound_info.irq=irqs[f_get_value(sound_win,40)];
  return 0;
  }


static void test_sound()
  {
  if (mixer_running) stop_mixing();
  if (sound_scan()) return;
  set_mixing_device(sound_info.device,22050,sound_info.port,sound_info.dma,sound_info.irq);
  start_mixing();
  change_music(test_sound_name);
  mix_back_sound(2);
  mix_back_sound(0);
  mixer_running=1;
  set_enable(0,70,mixer_running);
  }

static void stop_sound()
  {
  if (mixer_running)
     {
     stop_mixing();
     mixer_running=0;
     set_enable(sound_win,70,mixer_running);
     }
  }

void select_sound()
  {
  TSTR_LIST ls;
  int i;
  char buff[30];

  ls=create_list(8);
  default_font=&font6x9;
  for(i=0;i<8;i++)  str_add(&ls,device_name(i));
  str_replace(&ls,0,"<bez zvuku>");
  sound_win=def_dialoge(200,300,300,156,"Zvukov� karta",2);
  define(9,2,20,170,85,0,listbox,ls,0x03ff,0);c_default(sound_info.device);on_control_change(device_select2);
    property(def_border(0,0x4210),vga_font,NULL,WINCOLOR);
  define(-1,180,20,1,1,0,label,"Port:");
  define(20,10,20,30,12,1,input_line,3);property(def_border(0,0x4210),vga_font,NULL,WINCOLOR);
  set_default(itoa(sound_info.port,buff,16));
  define(-1,180,40,1,1,0,label,"DMA:");
  i=sound_info.dma;i-=(i>2)+(i>4);
  define(30,40,40,30,30,1,radio_butts,3,"0","1","3");c_default(i);on_control_change(change_dma);
  define(35,10,40,30,30,1,radio_butts,3,"5","6","7");c_default(i-3);on_control_change(change_dma);
  define(-1,180,80,1,1,0,label,"IRQ:");
  i=sound_info.irq;i-=2*(i>1)+(i>4)+(i>6);
  define(40,40,80,30,40,1,radio_butts,4,"2","3","5","7");c_default(i);
  define(50,10,5,80,20,2,button,"Test");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(test_sound);
  define(60,100,5,80,20,2,button,"Rozpoznat");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(detect_sound);
  define(70,190,5,80,20,2,button,"Stop");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(stop_sound);
  redraw_window();
  set_enable(0,70,mixer_running);
  device_select();
  }

static void back_start()
  {
  back=1;
  stop_sound();
  sound_scan();
  while (waktual!=NULL) close_current();
  terminate_gui();
  }

static void control_window(void *forward,void *back,void *help);
static void control_next1();

static void control_back1()
  {
  close_current();
  get_value(0,60,target_path);
  while (waktual!=NULL) close_current();
  exit_wait=0;
  select_vga();
  select_sound();
  control_window(control_next1,back_start,open_help);
  }

static trace_chdir(char *what)
  {
  char *c;
  char *d;

  c=alloca(strlen(what)+1);
  strcpy(c,what);
  d=strrchr(c,'\\');
  while (d!=NULL)
     {
     if (chdir(c)==0)
        {
        home_path(c);
        return;
        }
     *d=0;
     d=strrchr(c,'\\');
     }
  strcat(c,"\\");
  home_path(c);
  }


void static show_space(char device)
  {
  char buff[100];
  static long lastvalue;
  static char lastdevice;

  device=toupper(device);
  if (device>='A' && device<='Z')
     {
     if (device!=lastdevice) lastvalue=get_disk_free(device-'@')/1024;
     sprintf(buff,"Na disku %c: je voln�ch %d Kb",device,lastvalue);
     lastdevice=device;
     set_value(0,80,buff);
     }
  }

static open_load_window()
  {
  char c;
  get_value(0,60,target_path);
  trace_chdir(target_path);
  getcwd(target_path,2047);
  load_window();
  escape();
  home_path(source_path);
  c=o_aktual->id==150;
  if (c)
     {
     get_value(0,10,target_path);
     }
  close_current();
  if (c)
     set_value(0,60,target_path);
  else
     get_value(0,60,target_path);
  show_space(target_path[0]);
  }

static char create_ini(char *path,char *name)
  {

  if (inifile!=NULL)
     {
     fclose(ini);
     remove(inifile);
     free(inifile);
     }
  inifile=NewArr(char,strlen(path)+strlen(name)+2);
  sprintf(inifile,"%s\\%s",path,name);
  ini=fopen(inifile,"w");
  if (ini==NULL) return 1;
  add_to_list(inifile);
  fprintf(ini,"vmode %d\n", vmode);
  fprintf(ini,"sound_device %d %x %d %d\n",sound_info.device,sound_info.port,sound_info.dma,sound_info.irq);
  fprintf(ini,"sound_mixfreq 22049\n");
  fprintf(ini,"default_map lespred.map\n");
  fprintf(ini,"preload 1\n");
  fprintf(ini,"game_speed 5\n");
  return 0;
  }
  /*
static char create_setup_batch(char *path,char *name)
  {
  char *st;
  FILE *batch;

  st=alloca(strlen(path)+strlen(name)+2);
  sprintf(st,"%s\\%s",path,name);
  batch=fopen(st,"w");
  if (batch==NULL) return 1;
  fputs(
     "@Echo off\n"
     ":skok\n"
     "Echo Vloz CD \"Brany Skeldalu\" do mechaniky\n"
     "Echo a stiskni jakoukoliv klavesu. \n"
     "Echo.\n"
     "Echo Kombinace CNTR+C prerusi program....\n"
     "Pause > NUL\n",batch);
  fprintf(batch,"if not exist %s goto skok\n",pgm_name);
  fprintf(batch,"%s -s \n",pgm_name);
  fclose(batch);
  return 0;
  }
    */
static void close_ini()
  {
  if (ini!=NULL) fclose(ini);
  }

static void copy_self(char alloc)
  {
  static char *t;

  if (alloc)
     {
     t=NewArr(char,strlen(target_path)+strlen(SETUP_NAME)+2);
     sprintf(t,"%s\\%s",target_path,SETUP_NAME);
     cpy_file(pgm_name,t);
     }
  else free(t);
  }

static long self_size()
  {
  struct find_t f;

  _dos_findfirst(pgm_name,_A_NORMAL,&f);
  return f.size;
  }

static void start_install()
  {
  long diskfree;
  char error=0,autostart;
  get_value(0,60,target_path);
  if (find_object(waktual,70)!=NULL) autostart=f_get_value(0,70); else autostart=1;
  strupr(target_path);
  if (validate_path(target_path)==0)
     {
     msg_box("Cesta je �patn�",'\x1',"Cesta s c�lov�m adres��em je chybn� zad�na. Nepou��vejte dlouh� n�zvy z WINDOWS 95!","OK",NULL);
     return;
     }
  get_script(_SOURCE_,target_path);
  switch (o_aktual->id)
     {
     case 10:install_type=INST_MIN;break;
     case 20:install_type=INST_MED;break;
     case 30:install_type=INST_MAX;break;
     }
  maxcopy=install_sizes[install_type];
  diskfree=get_disk_free(target_path[0]-'@');
  if (maxcopy>diskfree)
     {
     if (msg_box("Nedostatek m�sta!",'\x1',"Instal�tor vypo��tal, �e BS zaberou v�ce m�sta ne� m�te na sv�m disku. "
                "Pokud v�ak pou��vate komprima�n� program jako je t�eba DRVSPACE, pak mohou b�t �daje o "
                "voln�m m�stu nep�esne. I p�es to, �e nen� voln� m�sto chcete pokra�ovat?","Ano","Ne",NULL)==2) return;
     }
  else if (maxcopy+1024*1024>diskfree)
     {
     if (msg_box("M�lo m�sta",'\x2',"Po nainstalov�n� by m�lo zb�t alespo� 1MB pro b�h. Sou�asn� prostor"
                 "umo��uje hru nainstalovat, av�ak hru nebude mo�n� spustit. Pokra�ovat?","Ano","Ne",NULL)==2) return;
     }
  maxcopy=maxcopy<<1;
  while (waktual!=NULL) close_current();
  cascade_mkdir(target_path);
  if (create_ini(target_path,SKELDAL_INI))
     {
     msg_box(PRG_HEADER,'\x1',"Nemohu vytvo�it konfigura�n� soubor. Zkontroluj spravnost z�pisu cesty","Ok",NULL);
     clean_up();
     return;
     }
//  create_setup_batch(target_path,SETUP_BATCH);
  display_progress();
  stop_sound();
  switch (install_type)
     {
     case INST_MIN:if (do_script(0) || do_script(-1) || do_script(-2) || do_script(-3)) error=1;break;
     case INST_MED:if (do_script(0) || do_script(1) || do_script(-2) || do_script(-3)) error=1;break;
     case INST_MAX:if (do_script(0) || do_script(1) || do_script(2) || do_script(3)) error=1;
                   break;
     }
  copy_self(1);
  cpy_flush();
  copy_self(0);
  close_ini();
  if (error) shutdown(),exit(1);
  close_current();
  do_events();
  if (!autostart) msg_box("Hotovo",' ',"Hra 'Br�ny Skeldalu' je �sp��n� nainstalov�na. Spu�t�n� prove�te z p��kazov� ��dky naps�n�m 'SKELDAL'","Konec",NULL);
  purge_file_list();
  shutdown();
  home_path(target_path);
  if (autostart)
     {
     puts("Chvilicku strpeni...");
     exit(254);
     }
  else exit(0);
  }

static EVENT_PROC (show_space_event)
  {
  OBJREC *o;

  o=(OBJREC *)user_ptr;
  if (msg->msg==E_KEYBOARD || msg->msg==E_CURSOR_TICK)
     {
     char *c;

     c=o->data;
     if (c[1]==':')
          show_space(c[0]);
     if (msg->msg==E_KEYBOARD)
        *(char *)msg->data=toupper(*(char *)msg->data);
     }
  }

static show_space_exit()
  {
  EVENT_MSG msg;

  msg.msg=E_KEYBOARD;
  show_space_event(&msg,(void **)o_aktual);
  }



static void rozsah_window()
  {
  char buff[100];
  char *text="Velikost na HD zhruba %d KB";

  exit_wait=0;
  default_font=&font6x9;
  def_dialoge(146,160,348,264,"Rozsah instalace",2);
  define(10,10,30,70,30,0,button,"Minim�ln�");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(start_install);
  define(-1,90,30,1,1,0,label,"Instaluje jen nejd�le�it�j�� soubory na HD");
  define(-1,90,40,1,1,0,label,"V�e ostatn� se pak na��t� p��mo z CD");
  sprintf(buff,text,install_sizes[0]/1024);
  define(-1,90,50,1,1,0,label,buff);
  define(20,10,80,70,30,0,button,"St�edn�");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(start_install);
  define(-1,90,80,1,1,0,label,"Instaluje v�echny datov� soubory na HD");
  define(-1,90,90,1,1,0,label,"Hudba a video se �tou p��mo z CD");
  sprintf(buff,text,install_sizes[1]/1024);
  define(-1,90,100,1,1,0,label,buff);
  define(30,10,130,70,30,0,button,"Maxim�ln�");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(start_install);
  define(-1,90,130,1,1,0,label,"Instaluje v�e na v�� HD v�etn� hudby");
  define(-1,90,140,1,1,0,label,"a videa. Z CD se nena��t� v�bec nic.");
  sprintf(buff,text,install_sizes[2]/1024);
  define(-1,90,150,1,1,0,label,buff);
  define(40,10,180,70,20,0,button,"Slo�ka");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(open_load_window);
  define(60,90,185,250,11,0,input_line,2048);property(def_border(1,0x4210),&font6x9,NULL,0x7fff);
     set_default(target_path);
     on_event(show_space_event);on_exit(show_space_exit);
  define(70,40,220,200,10,0,check_box,"Po nainstalov�n� hru automaticky spustit.");c_default(1);
  define(80,10,10,250,11,2,view_line,100);set_default("");
  show_space(target_path[0]);
  }

static void automatic_window()
  {
  char buff[100];
  char *text="Hra zabere zhruba %d KB m�sta";

  exit_wait=0;
  default_font=&font6x9;
  def_dialoge(110,200,420,80,"Instalovat kam?",2);
  define(40,10,33,70,15,0,button,"Naj�t");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(open_load_window);
  define(60,90,35,310,11,0,input_line,2048);property(def_border(1,0x4210),&font6x9,NULL,0x7fff);
     set_default(target_path);
     on_event(show_space_event);on_exit(show_space_exit);
  define(10,10,10,70,20,2,button,"Start");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(start_install);
  define(20,90,10,70,20,2,button,"<< Zp�t");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(back_start);
  sprintf(buff,text,install_sizes[0]/1024);
  define(90,180,20,200,10,2,view_line,100);set_default(buff);
  define(80,180,10,200,10,2,view_line,100);set_default("");
  show_space(target_path[0]);
  redraw_window();
  }



static void control_next1()
  {
  sound_scan();
  while (waktual!=NULL) close_current();
  rozsah_window();
  control_window(NULL,control_back1,open_help);
  }

static void control_window(void *forward,void *back,void *help)
  {
  def_dialoge(524,300,96,156,"Mo�nosti",2);
  define(10,8,30,80,20,0,button,"Dal�� >>");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(forward);
  define(20,8,60,80,20,0,button,"<< Zp�t");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(back);
  define(30,8,90,80,20,0,button,"? Pomoc");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(help);
  define(40,8,10,80,20,3,button,"Konec");property(bbutt,&font6x9,NULL,BUTTONCOLOR);
   if (setup_mode) on_control_change(stop_setup);else on_control_change(stop_copy);
  set_enable(0,10,forward!=NULL);
  set_enable(0,20,back!=NULL);
  set_enable(0,30,help!=NULL);
  redraw_window();
  }

static void run_setup()
  {
  int id;

  while (waktual!=NULL) close_current();
  get_script(_SOURCE_,"");
     {
     int minus[3];
     minus[1]=check_size(-2)+check_size(-3);
     minus[0]=minus[1]+check_size(-1);
     install_sizes[0]=check_size(0)+self_size();
     install_sizes[1]=install_sizes[0]+check_size(1);
     install_sizes[2]=install_sizes[1]+check_size(2)+check_size(3);
     install_sizes[0]+=minus[0];
     install_sizes[1]+=minus[1];
     id;
     }
  if (all_finder())
     {
     char *c;
     getcwd(&target_path,2047);
     c=strchr(target_path,0);
     c--;
     if (*c=='\\') *c=0;
     strcat(target_path,"\\"SKELDAL);
     }
  else
     {
     target_path[0]=select_disk()+'@';
     target_path[1]=':';
     target_path[2]='\\';
     strcpy(target_path+3,SKELDAL);
     }
  do
     {
     home_path(source_path);
     back=0;
     select_mode_win();
     about_window();
     id=o_aktual->id;
     close_current();
     close_current();
     exit_wait=0;
     if (id==10)
        {
        autodetect();
        automatic_window();
        }
     if (id==30)
        {
        if (msg_box("Autodetekce?",'\x1',"Chcete nejprve provest autodetekci hardware?","Ano","Ne",NULL)==1) autodetect();
        select_vga();
        select_sound();
        control_window(control_next1,back_start,open_help);
        }
     if (id!=40) escape();
     }
  while (back);
  }

static void save_ini()
  {
  char c[50];
  char d;
  char err=1;

  if (sound_scan()) return;
  d=msg_box("Potvrzen�?",'\x2',"Chce� zapsat zm�ny do konfigura�n�ho souboru hry?","Ano","Ne","Zru�it",NULL);
  if (d==3) return;
  if (d==1)
     {
     sprintf(c,"%d %x %d %d",sound_info.device,
                            sound_info.port,
                            sound_info.dma,
                            sound_info.irq);
     add_field_txt(&setup_ini,SOUND_DEVICE,c);
     add_field_num(&setup_ini,VMODE,vmode);
     home_path(target_path);
     err=save_config(setup_ini,SKELDAL_INI);
     }
  shutdown();
  home_path(target_path);
  switch (err)
     {
     case -1:puts("Pri ukladani se objevila neocekavana chyba. Soubor "SKELDAL_INI" nebyl ulozen");break;
     case 0:puts("Nastaveni bylo ulozeno..."); break;
     case 1:puts("Nastaveni nebylo zmeneno..."); break;
     }
  exit(0);
  }

static void deinstall()
  {
  char ig;
  int x,y;
  def_dialoge(200,200,250,156,"Odinstalovat?",2);
  define(-1,10,30,200,100,0,label,"Tato volba odstran� hru z va�eho disku.");
  define(-1,10,40,200,100,0,label,"Pot� ji� nebude mo�ne hru spustit");
  define(-1,10,50,200,100,0,label,"do nov�ho nainstalov�n� z CD.");
  define(10,30,80,150,10,0,check_box,"Zachovat ulo�en� pozice");c_default(1);
  define(20,10,10,80,30,3,button,"Ano");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  define(30,10,10,80,30,2,button,"Proboha NE");property(bbutt,&font6x9,NULL,BUTTONCOLOR);on_control_change(terminate_gui);
  redraw_window();
  escape();
  if (o_aktual->id==30)
     {
     close_current();
     return;
     }
  ig=f_get_value(0,10);
  set_enable(0,30,0);
  set_enable(0,20,0);
  set_enable(0,10,0);
  do_events();
  home_path(target_path);
  cascade_delete(ig);
  chdir("..");
  rmdir(target_path);
  home_path(source_path);
  curcolor=0;
  for(x=319;x>-1;x-=2)
    {
    y=x*240/320;bar(x,y,639-x,479-y);showview(0,0,0,0);
    }
  shutdown();
  exit(0);
  }

static void run_resetup()
  {
  int id;

  while (waktual!=NULL) close_current();
  do
     {
     home_path(source_path);
     back=0;
     select_mode_win_setup();
     about_window();
     id=o_aktual->id;
     close_current();
     close_current();
     exit_wait=0;
     if (id==30)
        {
        deinstall();
        back=1;
        }
     if (id==10)
        {
        select_vga();
        select_sound();
        control_window(save_ini,back_start,NULL);
        }
     if (id!=40) escape();
     }
  while (back);
  }


#define REPEAT(i,cnt) for(i=0;i<cnt;i++)

void warning()
  {
  int i,j;
  _settextcolor(14);
  _outtext("(C)1998 Napoleon gameS - Setup version 1.0 written by Ondrej Novak\n"
       "Instalacni program ke hre Brany Skeldalu\n\nUpozorneni:\n");
  _settextcolor(15);
  _outtext("�"); REPEAT(i,78) _outtext("�");_outtext("�");
  REPEAT(j,5)
     {
     _outtext("�"); REPEAT(i,78)_outtext(" ");_outtext("�");
     }
  _outtext("�"); REPEAT(i,78) _outtext("�");_outtext("�");
  _settextcolor(13);
  _settextwindow(6,2,20,79);
  _outtext("Stiskem jakekoliv  klavesy spustite instalacni program. Pokud  potom obrazovka"
           "zcerna, nebo  zobrazi nesmyslny obrazec stisknete klavesu ESC. Tim dojde k na-"
           "prave zobrazeni  a vy  budete moci  pokracovat v  instalovani hry. Kdyz nebude"
           "pocitac reagovat, restartujte jej a spustte tento program znova.\n"
           "V pripade problemu s instalatorem nahlednete do prirucky hry.");

  }

static void test_mode_xxx()
  {
  if (test_mode()) run_setup();
  }

static int ask_video_win=-1;

static void ask_video()
  {
  if (find_window(ask_video_win)!=NULL) return;
  default_font=&font6x9;
  ask_video_win=def_dialoge(242,100,156,156,"Vyber grafiku",3);
  define(9,0,20,156,80,0,listbox,video_ls,0x03ff,0);c_default(vmode);
    property(NULL,vga_font,NULL,WINCOLOR);
  define(20,5,5,60,20,2,button,"Konec");on_control_change(terminate_gui);
    property(bbutt,NULL,NULL,BUTTONCOLOR);
  define(20,70,5,60,20,2,button,"OK");on_control_change(test_mode_xxx);
    property(bbutt,NULL,NULL,BUTTONCOLOR);
  redraw_window();
  }

static EVENT_PROC(esc_mode)
  {
  GET_USER_PTR();
  WHEN_MSG(E_KEYBOARD)
     {
     if (GET_DATA(char)==27)
        {
        donegr();
        initgr_low();
        zobraz_mysku();
        rescue_mode=1;
        redraw_desktop();
        vmode=0;
        terminate_gui();
        }
     }
  }


static void about_window()
  {
  char r=!rescue_mode;
  if (r) send_message(E_ADD,E_KEYBOARD,esc_mode);
  def_dialoge(200,100,240,144,PRG_HEADER,2);
  define(-1,10,30,220,10,0,mid_label,"V�tej u instal�toru hry:");
  define(-1,10,42,220,10,0,mid_label,"Br�ny Skeldalu");
  define(-1,10,80,220,10,0,mid_label,"Instala�n� program napsal:");
  define(-1,10,92,220,10,0,mid_label,"Ond�ej Nov�k");
  define(-1,10,110,220,10,0,mid_label,"(C)1998 Napoleon gameS");
  redraw_window();
  escape();
  if (r) send_message(E_DONE,E_KEYBOARD,esc_mode);
  }

void skladba_konec(char **name)
  {
  static char xxx[]="?";
  *name=xxx;
  }

void check_setup_mode(int argc,char **argv)
  {
  if ((argc>=2 && argv[1][0]=='-' && toupper(argv[1][1])=='S') || access(SKELDAL_INI,F_OK)==0)
     {
     char *c;
     char *d;
     setup_mode=1;
     getcwd(target_path,PATH_MAX+1);
     setup_ini=read_config(SKELDAL_INI);
     if (setup_ini==NULL)
        {
        printf("The file %s has not been found. You must reinstall the game\n"
               "Insert the CD into driver and run INSTALL.EXE\n",SKELDAL_INI);
        printf("Nemohu najit soubor %s. Budes muset hru znovu nainstalovat\n"
               "Z CD \"Brany Skeldalu\" spust program INSTALL.EXE\n",SKELDAL_INI);
        exit(0);
        }
     get_num_field(setup_ini,VMODE,&vmode);
		 c=get_text_field(setup_ini,"CESTA_CD");
     program_data_path=NewArr(char,strlen(c)+1);strcpy(program_data_path,c);
     c=strchr(program_data_path,0);
     if (c>program_data_path && c[-1]=='\n') c[-1]=0,c--;
     if (c>program_data_path && c[-1]=='\\') c[-1]=0;
     c=get_text_field(setup_ini,SOUND_DEVICE);
     sscanf(c,"%d %x %d %d",&sound_info.device,
                            &sound_info.port,
                            &sound_info.dma,
                            &sound_info.irq);
     c=alloca(strlen(argv[0])+1);
     strcpy(c,argv[0]);
     d=strrchr(c,'\\');
     if (d!=NULL)
        {
        d[d-c<=2]=0;
        home_path(c);
        }
     }
  }

main(int argc,char **argv)
  {
  char s;
  char *c;

  pgm_name=argv[0];
  check_setup_mode(argc,argv);
  if (argc==2 && !setup_mode) copy_source_path=argv[1];else copy_source_path=NULL;
  if (!setup_mode)
     {
     _setvideomode(_TEXTC80);
     rescue_mode=!(s=create_er_file());
     if (s && !setup_mode) warning(),getche();
     }
  else
     s=0;
  init_setup(s);
  create_video_ls();
  set_mixing_device(0,22050,0,0,0);
  c=test_sound_name=NewArr(char, strlen(program_data_path)+strlen(TEST_MUSIC)+1);
  sprintf(c,"%s%s",program_data_path,TEST_MUSIC);
  if (access(c,F_OK)!=0)
    {
    shutdown();
    printf("Vloz CD \"Brany Skeldalu\" do jednotky CD-ROM a spust znova instalaci\n"
           "Insert CD \"The Gates of the Skeldal\" into CD-ROM and try to run INSTALL again.\n");
    abort();
    }
  konec_skladby=skladba_konec;
  change_music(c);
  mix_back_sound(2);
  if (setup_mode) run_resetup();
  else
     {
     if (!rescue_mode) run_setup();
     if (rescue_mode)
        {
        ask_video();
        escape();
        }
     }
  shutdown();
  }
