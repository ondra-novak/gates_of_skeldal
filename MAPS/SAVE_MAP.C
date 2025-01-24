#include <skeldal_win.h>
#include <stdio.h>
#include <time.h>
#include <mem.h>
#include <malloc.h>
#include <stdlib.h>
#include <types.h>
#include <memman.h>
#include <event.h>
#include <devices.h>
#include <bmouse.h>
#include <bgraph.h>
#include <gui.h>
#include <basicobj.h>
#include <strlite.h>
#include <strlists.h>
#include <direct.h>
#include <fcntl.h>
#include <dos.h>
#include "mapy.h"
#include "edit_map.h"
#include "globals.h"
#include "steny.h"
#include "mob_edit.h"
#include "save_map.h"
#include <io.h>

#define A_SIDEMAP 0x8001
#define A_SECTMAP 0x8002
#define A_STRTAB1 0x8003 //stena main
#define A_STRTAB2 0x8004 //stena leva
#define A_STRTAB3 0x8005 //stena prava
#define A_STRTAB4 0x8006 //strop
#define A_STRTAB5 0x8007 //podlaha
#define A_STRTAB6 0x8008 //oblouk levy
#define A_MAPINFO 0x8009
#define A_MAPGLOB 0x800A
#define A_STRTAB7 0x800B //oblouk pravy
#define A_MAPITEM 0x800C //itemy
#define A_MAPMACR 0x800D //Makra multiakce
#define A_MAPFLY  0x800E //Letajici predmety
#define A_MAPMOBS 0x800F //Potvory v bludi¨ti
#define A_MAPVYK  0x8010 //Vyklenky
#define A_MOBS    0x8011 //Potvory definice
#define A_MOBSND  0x8012 //Potvory zvuky
#define A_PASSW   0x8013 //Heslo 1
#define A_PASSW2  0x8014 //Heslo 2
#define A_MAPEND  0x8000

#define BACKUP_ADR "backup"

char filename[MAX_PATH];
  static char backup=0;

static char password[50];
static char passpass;

void encrypt(char *password)
  {
  char *p=password;

  p=password;
  while(*p) *p=~*p,p++;
  }

void encrypt2(char *in,char *out,int size)
  {
  int c=clock(),i;
  char z;
  for(i=0;i<size;i++) out[i]=(rand()*256/(RAND_MAX+1));
  memcpy(out,&c,sizeof(int));
  out+=4;
  srand(c);
  do
    {
    z=*in;
    *out++=z+(rand()*256/(RAND_MAX+1));
    }
  while(*in++);
  }

void decrypt2(char *in,char *out)
  {
  int c;
  char z;
  memcpy(&c,in,sizeof(int));
  in+=4;
  srand(c);
  if (c==0)
    {
    strcpy(out,"\x8\n");
    return;
    }
  do
    {
    z=*in++;
    *out=z-(rand()*256/(RAND_MAX+1));
    }
  while(*out++);
  }


long load_section(FILE *f,void **section, int *sct_type,long *sect_size)
  {
  long s;
  char c[20];

  *section=NULL;
  fread(c,1,sizeof(sekceid),f);
  if (strcmp(c,sekceid)) return -1;
  fread(sct_type,1,sizeof(*sct_type),f);
  fread(sect_size,1,sizeof(*sect_size),f);
  fread(&s,1,sizeof(s),f);
  *section=getmem(*sect_size);
  s=fread(*section,1,*sect_size,f);
  return s;
  }

void *build_sample_list(long *size)
  {
  PTRMAP *q;
  int i;
  TSMP_STAMP *z;

  q=smplist;i=0;
  if (q==NULL) return NULL;
  while(q!=NULL)
     i++,q=q->next;
  *size=i*sizeof(TSMP_STAMP);
  z=(TSMP_STAMP *)getmem(*size);
  i=0;
  q=smplist;
  while(q!=NULL)
     memcpy(&z[i++],q->data,sizeof(TSMP_STAMP)),q=q->next;
  return z;
  }

void build_sample_tree(TSMP_STAMP *z,long size)
  {
  int i;

  smplist=NULL;
  for (i=0;i<size/sizeof(TSMP_STAMP);i++)
     pl_add_data(&smplist,&z[i],sizeof(TSMP_STAMP));
  }

void purge_mob_map();

/*static void Shift_map_sectors()
  {
  int i;
  for (i=0;i<maplen;i++) minfo[i].layer^=0x80;
  }*/

static void Shift_map_sectors_up()
  {
  int i;
  for (i=0;i<maplen;i++) minfo[i].layer+=40;
  }

static void Shift_map_sectors_down()
  {
  int i;
  for (i=0;i<maplen;i++) minfo[i].layer-=40;
  }

int load_map(char *filename)
  {
  FILE *f;
  void *temp;
  int sect,i;
  long size,r;
  char nmapend=1;
  static char las=1;
  static char newcode=0;

  passpass=0;
  purge_mob_map();
  if (las)
     {
     if (check_sound_map())
        msg_box("Upozornˆn¡!",'\x1',"Verze souboru SOUND.DAT neodpovid  verz¡m ostatn¡ch soubor–. To m–‘e zp–sobit poru¨en¡ integrity ukazatel– na zvukov‚ soubory!","Pokra‡ovat",NULL);
     load_all_shops();
     las=0;
     }
  load_mobs();
  load_sound_map();
  load_items();
  password[0]=0;
  f=fopen(filename,"rb");
  if (f==NULL) return 0;
  do
     {
     r=load_section(f,&temp,&sect,&size);
     if (r==size)
        switch (sect)
         {
         case A_SIDEMAP:
                  memcpy(&mapa.sidedef,temp,size);
                  break;
         case A_SECTMAP:
                  memcpy(&mapa.sectordef,temp,size);
                  break;
         case A_MAPGLOB:
                  memset(&mglob,0,sizeof(mglob));
                  if (size>sizeof(mglob)) size=sizeof(mglob);
                  memcpy(&mglob,temp,size);
/*                  if (size>sizeof(mglob)-sizeof(mglob.mappassw))
                    {
                    decrypt2(mglob.mappassw,password);
                    passpass=2;
                    if (password[0]) newcode=1;
                    }*/
                  break;
         case A_MAPINFO:
                  memcpy(&minfo,temp,size);
                  maplen=size / sizeof(TMAP_EDIT_INFO);
                  sect=0;
                  for (i=0;i<maplen;i++) sect+=minfo[i].layer;
                  if (sect/maplen<-20) Shift_map_sectors_up();
                  if (sect/maplen>20)  Shift_map_sectors_down();
                  break;
         case A_MAPMACR:
                  load_macros(temp);
                  break;
         case A_MAPEND:
                  nmapend=0;
                  break;
         case A_MAPITEM:
                  load_item_map(temp,size);
                  break;
         case A_MAPMOBS:
                  load_mob_map(temp,size);
                  break;
         case A_MOBS:
                  load_mobs_to_map(temp,size);
                  mglob.local_monsters=1;
                  break;
         case A_MOBSND:
                  load_sound_dat(temp,size);
                  break;
         case A_MAPVYK:
                  memcpy(vyklenky,temp,size);
                  break;
         case A_PASSW:
/*                  if (passpass<2)
                    {
                    memcpy(password,temp,size);
                    encrypt(password);
                    passpass=2;
                    }*/
                  break;
        }
     else
        {
        if (temp!=NULL)free(temp);
        fclose(f);
        return -1;
        }
     free(temp);
     }
  while (nmapend);
  fclose(f);
  for(r=1;r<maplen;r++)
     for(i=0;i<4;i++)
     if (mapa.sidedef[r][i].flags & 0x40000)
        if ((minfo[r].x+minfo[r].y+i)& 1) mapa.sidedef[r][i].prim-=(mapa.sidedef[r][i].prim_anim & 0xf)+1;
  backup=0;
  if (newcode) Shift_map_sectors_down();
  return 0;
  }

long save_section(FILE *f,void *section, int sct_type,long sect_size)
  {
  long s;

  fwrite(sekceid,1,sizeof(sekceid),f);
  fwrite(&sct_type,1,sizeof(sct_type),f);
  fwrite(&sect_size,1,sizeof(sect_size),f);
  s=ftell(f)+sect_size+sizeof(s);
  fwrite(&s,1,sizeof(s),f);
  s=fwrite(section,1,sect_size,f);
  return s;
  }

int save_scr_list(FILE *f,TSTR_LIST names,int scr_id)
  {
  long bytesize;
  int cn;
  char *c,*p;
  int nerr,j;

     bytesize=0;j=1;
     cn=str_count(names);
     while (cn>0 && names[cn-1]==NULL) cn--;
     while (j<cn)
        {
        if (names[j]!=NULL) bytesize+=strlen(names[j])+1;else bytesize++;
        j++;
        }
     if (!bytesize) return 1;
     c=(char *)getmem(bytesize);
     j=1;p=c;
     while (j<cn)
        {
        if (names[j]!=NULL) strcpy(p,names[j]);else *p=0;
        p=strchr(p,'\0');p++;
        j++;
        }
        nerr=(bytesize==save_section(f,c,scr_id,bytesize));//A_STRTAB1+i-1,bytesize));
     free(c);
  return nerr;
  }


void backup_info(EVENT_MSG *msg)
  {
  char *c;
  if (msg->msg==E_INIT) return ;
  if (msg->msg==E_DONE) return ;
  if (msg->msg==E_MOUSE)
     {
     MS_EVENT *ms;

     ms=get_mouse(msg);
     if (ms->tl1)
        {
        send_message(E_STATUS_LINE,E_DONE,E_IDLE,backup_info);
        send_message(E_DONE,E_MOUSE,backup_info);
        return;
        }
     return;
     }
  c=(char *)msg->data;
  strcpy(c,"Star˜ soubor byl ulo‘en v adres ©i backup\\...");
  c=strchr(c,'\0');
  msg->data=(void *)c;
  msg->msg=-1;
  return;
  }


void create_backup(char *old_filename)
  {
  char *c;
  FILE *fl;
  void *d;
  long siz;

  mkdir(BACKUP_ADR);
  c=alloca(strlen(BACKUP_ADR)+1+strlen(old_filename)+1);
  sprintf(c,"%s\\%s",BACKUP_ADR,old_filename);
  fl=fopen(old_filename,"rb");if (fl==NULL) return;
  fseek(fl,0,SEEK_END);siz=ftell(fl);
  fseek(fl,0,SEEK_SET);
  d=getmem(siz);
  fread(d,1,siz,fl);
  fclose(fl);
  fl=fopen(c,"wb");
  if (fl!=NULL)
     {
     fwrite(d,1,siz,fl);
     fclose(fl);
     send_message(E_STATUS_LINE,E_ADD,E_IDLE,backup_info);
     send_message(E_ADD,E_MOUSE,backup_info);
     }
  free(d);
  }




int save_map(char *filename)
  {
  long bytesize;
  FILE *f;
  char nerr=1;char *pr;
  int i,j;
  TSTR_LIST names=NULL;
  void *temp;
  char enpass[58];

  f=fopen(filename,"wb");
  if (f==NULL) return -1;

  for (i=1;i<maplen;i++)
     for (j=0;j<4;j++)
     {
     TSTENA *w;

     w=&mapa.sidedef[i][j];
     if (w->flags & 0x40000)
        if ((minfo[i].x+minfo[i].y+j)& 1)
          w->prim+=(w->prim_anim & 0xf)+1;
     }

  bytesize=sizeof(mglob);
  encrypt2(password,mglob.mappassw,sizeof(mglob.mappassw));
  if (nerr)
     nerr&=(bytesize==save_section(f,&mglob,A_MAPGLOB,bytesize));

  bytesize=maplen*sizeof(TSTENA)*4;
  if (nerr)
     nerr&=(bytesize==save_section(f,&mapa.sidedef,A_SIDEMAP,bytesize));

  bytesize=maplen*sizeof(TSECTOR);
  if (nerr)
     nerr&=(bytesize==save_section(f,&mapa.sectordef,A_SECTMAP,bytesize));

  if (password[0]) Shift_map_sectors_up();
  bytesize=maplen*sizeof(TMAP_EDIT_INFO);
  if (nerr)
     nerr&=(bytesize==save_section(f,&minfo,A_MAPINFO,bytesize));
  if (password[0]) Shift_map_sectors_down();
  pr=pripona(filename,SCR);
  for (i=1;i<8;i++)
     {

     switch (i)
        {
        case 1:
        case 2:
        case 3:read_side_script_one(pr,NSID,&names,i,4);break;
        case 4:read_side_script_one(pr,NCEI,&names,1,2);break;
        case 5:read_side_script_one(pr,NFLR,&names,1,2);break;
        case 6:read_side_script_one(pr,NOBL,&names,1,3);break;
        case 7:read_side_script_one(pr,NOBL,&names,2,3);break;
        }
     if (nerr)
     nerr&=save_scr_list(f,names,i==7?A_STRTAB7:A_STRTAB1+i-1);
     }

  strcpy(enpass,"Heslo \x8 je heslo");
  bytesize=strlen(enpass)+1;
    if (nerr && bytesize>1)
      {
      encrypt(enpass);
      memcpy(enpass+bytesize,"<BLOCK>\0\x13\x80",12);bytesize+=24;
      nerr&=(bytesize==save_section(f,enpass,A_PASSW,bytesize));
      }
  temp=save_macros(&bytesize);
   if (temp!=NULL)
     {
    if (nerr)
     nerr&=(bytesize==save_section(f,temp,A_MAPMACR,bytesize));
     free(temp);
     }

  if (nerr)
     nerr&=save_item_map(f,A_MAPITEM);

  if (mglob.local_monsters)
     {
     temp=save_mobs_to_map(&bytesize);
        if (nerr && temp!=NULL)
           nerr&=(bytesize==save_section(f,temp,A_MOBS,bytesize));
     free(temp);
     nerr&=save_sound_dat(f,A_MOBSND);
     }

  temp=save_mob_map(&bytesize);
   if (temp!=NULL)
     {
    if (nerr)
     nerr&=(bytesize==save_section(f,temp,A_MAPMOBS,bytesize));
     free(temp);
     }
  for(i=255;i>1;i--) if (vyklenky[i].sector!=0)  break;
  bytesize=sizeof(TVYKLENEK)*(i+1);
    if (nerr)
     nerr&=(bytesize==save_section(f,vyklenky,A_MAPVYK,bytesize));
  strcpy(enpass,"To bys \x8 chtel vedet co? \x8 ");
  bytesize=strlen(enpass)+1;
    if (nerr && bytesize>1)
      {
      encrypt(enpass);
      memcpy(enpass+bytesize,"<BLOCK>\0\x13\x80",12);bytesize+=12;
      nerr&=(bytesize==save_section(f,enpass,A_PASSW,bytesize));
      }
  bytesize=sizeof(bytesize);
    if (nerr)
     nerr&=(bytesize==save_section(f,&bytesize,A_MAPEND,bytesize));
  strcpy(enpass,"Ty si \x8 ale nedas pokoj! \x8");
  bytesize=strlen(enpass)+1;
    if (nerr && bytesize>1)
      {
      encrypt(enpass);
      memcpy(enpass+bytesize,"<BLOCK>\0\x13\x80",12);bytesize+=12;
      nerr&=(bytesize==save_section(f,enpass,A_PASSW,bytesize));
      }
  bytesize=sizeof(bytesize);
    if (nerr)
     nerr&=(bytesize==save_section(f,&bytesize,A_MAPEND,bytesize));
  fclose(f);
  if (names!=NULL) release_list(names);
  {
  int x,y;
  for(x=1;x<maplen;x++)
     for(y=0;y<4;y++)
     if (mapa.sidedef[x][y].flags & 0x40000)
        if ((minfo[x].x+minfo[x].y+y)& 1) mapa.sidedef[x][y].prim-=(mapa.sidedef[x][y].prim_anim & 0xf)+1;
  }
  return (int)nerr-1;
  }

int check_map(int *objnumber,int *objsub)
  {
  int i,j;
	int maxp;

	maxp=str_count(side_names);
  for (i=1;i<maplen;i++)
     for (j=0;j<4;j++)
       if (mapa.sectordef[i].sector_type)
     {
     TSTENA *w;
     TSECTOR *s;
     w=&mapa.sidedef[i][j];
     s=&mapa.sectordef[i];
     minfo[i].flags &=~1;
     w->prim_anim&=0xf;
     w->sec_anim&=0xf;
     if (!s->step_next[j])
        {
        w->flags|=0x1e;
        w->flags&=~0x080;
        }
     if (!w->sector_tag)
        {
        w->action=0;
        w->side_tag&=~3;
        }
     if (w->flags & 0x100)
        {
        w->prim_anim|=(rand()*(w->prim_anim)/32768)<<4;
        }
		 if (w->prim>=maxp) w->prim=1;
/*     if (!(w->flags & 0x80) && w->prim==0)
        {
        *objnumber=i;
        *objsub=j;
        return -1; // Wall definition missing
        }                                    */
     if (s->step_next[j] && !(mapa.sectordef[s->step_next[j]].sector_type))
        {
        *objnumber=i;
        *objsub=j;
        return -2; // Wall leades to unexisting sector
        }
/*     if (s->sector_type==3)
        if (w->flags & 1)
        if (mapa.sidedef[i][(j+1)&3].flags & 1 || mapa.sidedef[i][(j-1)&3].flags & 1 ||
        (!(mapa.sidedef[i][(j^2)].flags & 1)))
        {
        *objnumber=i;
        *objsub=-1;
        return -3; // Stairs wire error.
        }                               */
     if (chka[w->action] && !(mapa.sectordef[w->sector_tag].sector_type))
        {
        *objnumber=i;
        *objsub=j;
        return -4; // Event leades to unexisting sector.
        }
     }
  return 0;
  }

static char set_date_time(char *filename, unsigned long date, unsigned long time)
  {
  HANDLE h=CreateFile(filename,0,0,NULL,OPEN_EXISTING,0,NULL);
  FILETIME WriteTime;
  char err=0;
  if (h==NULL) return 1;
  WriteTime.dwHighDateTime=date;
  WriteTime.dwLowDateTime=time;
  err=SetFileTime(h,NULL,NULL,&WriteTime)==1;
  CloseHandle(h);
  return err;
  }

static char get_date_time(char *filename, unsigned long *date, unsigned long *time)
  {
  HANDLE h=CreateFile(filename,0,0,NULL,OPEN_EXISTING,0,NULL);
  FILETIME WriteTime;
  char err=0;
  if (h==NULL) return 1;
  err=GetFileTime(h,NULL,NULL,&WriteTime)!=1;
  *date=WriteTime.dwHighDateTime;
  *time=WriteTime.dwLowDateTime;
  CloseHandle(h);
  return err;
  }

void validate_sound_map()
  {
   unsigned short date,time;

  if (get_date_time(MOB_SOUND,&date,&time)) return;
  set_date_time(MOB_FILE,date,time);
  set_date_time(ITEM_FILE,date,time);
  }

char check_sound_map()
  {
   unsigned long date,time;
   unsigned long cdate,ctime;

  if (get_date_time(MOB_SOUND,&date,&time)) return 0;
  get_date_time(MOB_FILE,&cdate,&ctime);
  if (date!=cdate || time!=ctime) return 1;
  get_date_time(ITEM_FILE,&cdate,&ctime);
  if (date!=cdate || time!=ctime) return 1;
  return 0;
  }


int save_all_map(void)
  {
  int x,y,z;
  char info[160];
  char *txt;
  if ((z=check_map(&x,&y))!=0)
     {
     WINDOW *w;
     unselect_map();
     if ((w=find_window(tool_bar))!=NULL) close_window(w);
     tool_sel=30;
     create_map_win(-1);
     open_sector_win();
     jdi_na_sektor(x);
     }
  sprintf(info,"Chyba %02d na pozici %d:%d",-z,x,y);
  switch (z)
     {
     case -1:msg_box(info,'\01',"Chyb¡ definice stˆny!","OK",NULL);break;
     case -2:msg_box(info,'\01',"Chodba vede do neexistuj¡c¡ho sektoru!","OK",NULL);break;
     case -3:msg_box(info,'\01',"Schody jsou ¨patnˆ spojen‚ se sousedn¡m¡ sektory!","OK",NULL);break;
     case -4:msg_box(info,'\01',"Ud lost v neexistuj¡c¡m sektoru!","OK",NULL);break;
     }
  if (!backup)
     {
     create_backup(filename);
     create_backup(SHOP_NAME);
     create_backup(ITEMS_DAT);
     create_backup(MOB_FILE);
     create_backup(MOB_SOUND);
     backup=1;
     }
  if (save_map(filename))
     {
     sprintf(info,"Nedok ‘u ulo‘it soubor %s.",filename);
     msg_box("Chyba I/O",'\01',info,"!Panika!",NULL);
     }
  else txt=pripona(filename,TXT);
  save_items();
  if (_access(txt,0)!=0)
    {
    FILE *f;

    f=fopen(txt,"w");
    if (f) {
        fputs("-1\n",f);
        fclose(f);
    }
    }
  if (!mglob.local_monsters)
     {
     save_mobs();
     save_sound_map();
     }
  save_all_shops();
  validate_sound_map();
  return z;
  }

char *set_password(char *text)
  {
  if (text!=NULL)
    {
    strncpy(password,text,sizeof(password));
    password[sizeof(password)-1]=0;
    }
  return password;
  }

char check_password(char *text)
  {
  if (password[0]==0) return 1;
  if (text==NULL) return 0;
  return !strcmp(text,password);
  }


