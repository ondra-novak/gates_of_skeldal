#include <libs/types.h>
#include <ctype.h>
#include <io.h>
#include <dos.h>
#include <malloc.h>
#include <direct.h>
#include <libs/memman.h>
#include <string.h>
#include <libs/event.h>
#include <libs/strlite.h>
#include <stdio.h>
#include <ctype.h>
#include <libs/bgraph.h>
#include <vesa.h>
#include <libs/bmouse.h>
#include <libs/gui.h>
#include <libs/basicobj.h>
#include "setuplib.h"
#include "setupcpy.h"
#include "setup.h"

#define COPY "COPY"
#define MKDIR "MKDIR"
#define INI "INI"

#define DIR_NAMES 6
char *dirnames[]=
  {
  "GAMES.*",
  "GAME*.*",
  "HRY.*",
  "ZABAVA.*",
  "SKELDAL",
  "NAPOLEON",
  };

TSTR_LIST file_list=NULL;

static void done_bar_init(OBJREC *o,long *params)
  {
  o->userptr=New(long);
  *(long *)o->userptr=*params;
  }

static void done_bar_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  long value,max,x3;

  value=*(long *)o->data;
  max=*(long *)o->userptr;
  x3=x1+(x2-x1)*value/max;
  if (x3<=x1) x3=x1;
  if (x3>=x2) x3=x2;
  bar(x3,y1,x2,y2);
  curcolor=o->f_color[1];
  bar(x1,y1,x3,y2);
  }

void done_bar(OBJREC *o) //define(...done_bar,max);
  {
  o->call_init=done_bar_init;
  o->call_draw=done_bar_draw;
  o->datasize=4;
  }



char select_disk()
  {
  char last=3;
  struct diskfree_t ds;
  int max=0,i;
  long now;

  for(i=('C'-'@');i<('Z'-'@');i++)
     {
     if (_dos_getdiskfree(i,&ds)==0)
        {
        now=ds.avail_clusters*ds.sectors_per_cluster*ds.bytes_per_sector;
        if (now>max)
           {
           max=now;
           last=i;
           }
        }
     }
  return last;
  }

char dir_finder()
  {
  struct find_t ft;
  int i;
  int rc;

  for(i=0;i<DIR_NAMES;i++)
     {
     rc=_dos_findfirst(dirnames[i],_A_SUBDIR,&ft);
     if (rc==0 && ft.attrib & _A_SUBDIR) break;
     do_events();
     }
  if (i==DIR_NAMES) return 0;
  if (strcmp(dirnames[i],"SKELDAL"))
     {
     chdir(ft.name);
     }
  return 1;
  }

long get_disk_free(char disk)
  {
  struct diskfree_t ds;
  if (_dos_getdiskfree(disk,&ds)==0)
     return ds.avail_clusters*ds.sectors_per_cluster*ds.bytes_per_sector;
  return 0;
  }


char all_finder()
  {
  unsigned total;
  int i,j,max,lmax,p;
  long sz[30];
  int end='Z'-'@';

  exit_wait=0;
  lmax=0x7fffffff;
  for(i=3;i<end;i++) sz[i]=get_disk_free(i);
  for(i=3;i<end;i++)
     {
     p=3;max=0;
     for(j=3;j<end;j++) if (sz[j]>max && sz[j]<lmax) p=j,max=sz[j];
     lmax=max;
     _dos_setdrive(p,&total);
     chdir("\\");
     if (dir_finder()) return 1;
     }
  return 0;
  }

TSTR_LIST script=NULL;
char work_buff[8000];

static char compile_script(char *in,char *out,char *source,char *target)
  {
  char *c,*d;

  c=in;d=out;
  while (*c)
     {
     if (*c=='%')
        switch(toupper(*(++c)))
           {
           case 'T':strcpy(d,target);d=strchr(d,0);break;
           case 'S':strcpy(d,source);d=strchr(d,0);break;
           default: return 1;
           }
     else
        *d++=*c;
     c++;
     }
  *d=0;
  return 0;
  }

char read_script(char *filename,char *source_path,char *target_path)
  {
  FILE *f;
  char buff[161];
  int i=0;

  if (script!=NULL) release_list(script);
  script=create_list(16);
  f=fopen(filename,"r");
  if (f==NULL) return 1;
  while (!feof(f))
     {
     if (fgets(buff,160,f)==NULL) break;
     if (buff[strlen(buff)-1]=='\n') buff[strlen(buff)-1]=0;
     if (buff[0]==0) continue;
     if (compile_script(buff,work_buff,source_path,target_path))
        {
        fclose(f);
        return 2;
        }
     str_replace(&script,i,work_buff);
     i++;
     }
  fclose(f);
  return 0;
  }

long check_size(int gr)
  {
  int group,pocet,i;
  char prikaz[20];
  long suma=0;

  pocet=str_count(script);
  for(i=0;i<pocet;i++) if (script[i]!=NULL)
     {
     if (sscanf(script[i],"%d %s %s",&group,prikaz,work_buff)<1) group=gr+1;
     strupper(prikaz);
     if (!strcmp(prikaz,COPY) && group==gr)
        {
        struct find_t ft;
        int rc;

        rc=_dos_findfirst(work_buff,_A_NORMAL,&ft);
        while (rc==0)
           {
           suma+=ft.size;
           rc=_dos_findnext(&ft);
           }
        }
     }
  return suma;
  }



static void view_line_init(OBJREC *o,int *len)
  {
  o->datasize=(*len)+1;
  }

static void view_line_draw(int x1,int y1, int x2, int y2, OBJREC *o)
  {
  char *c;

  bar(x1,y1,x2,y2);
  c=(char *)o->data;
  if (!*c) return;
  set_aligned_position(x2,y1,2,0,c);
  outtext(c);
  }


void view_line(OBJREC *o)
  {
  o->call_init=view_line_init;
  o->call_draw=view_line_draw;
  }


char get_max_res()
  {
  if (vesasupport(0x110)) return 2;
  if (vesasupport(0x100)) return 1;
  return 0;
  }

void add_to_list(char *name)
  {
  if (file_list==NULL) file_list=create_list(256);
  str_add(&file_list,name);
  }

static void copy_1_file(char *init_mask,char *filename,char *target)
  {
  char *c,*d;

  c=alloca(strlen(init_mask)+strlen(filename)+1);
  strcpy(c,init_mask);
  d=strrchr(c,'\\');
  d++;
  strcpy(d,filename);
  d=alloca(strlen(target)+strlen(filename)+10);
  strcpy(d,target);
  if (d[0]==0 || d[strlen(d)-1]!='\\') strcat(d,"\\");
  strcat(d,filename);
  set_value(0,20,filename);
  add_to_list(d);
  cpy_file(c,d);
  }

static void copy_files(char *param)
  {
  struct find_t ft;
  char *source_mask;
  char *target;
  int rc;

  target=strchr(param,' ');
  if (target==NULL) return;
  source_mask=alloca(target-param);
  source_mask[target-param]=0;
  strncpy(source_mask,param,target-param);
  target++;
  rc=_dos_findfirst(source_mask,_A_NORMAL,&ft);
  while (rc==0)
     {
     copy_1_file(source_mask,ft.name,target);
     rc=_dos_findnext(&ft);
     }
  _dos_findclose(&ft);
  }

char cascade_mkdir(char *path)
  {
  char *c;
  char d;

  if (path[0]==0||path[1]==':' && path[2]==0) return 0;
  if (!access(path,F_OK)) return 0;
  c=strrchr(path,'\\');
  if (c==NULL) return 0;
  *c=0;
  d=cascade_mkdir(path);
  *c='\\';
  if (d==1) return 1;
  if (mkdir(path)!=0) return 1;
  add_to_list(path);
  return 0;
  }


static char add_to_ini(char *params)
  {
  return fprintf(ini,"%s\n",params)<0;
  }

static char *commands[]=
  {
  COPY,
  MKDIR,
  INI,
  };

static char (*calls[])(char *params)=
  {
  copy_files,
  cascade_mkdir,
  add_to_ini,
  };



static char command_match(char *cmd,char *test,char **params)
  {
  while (*test) if (toupper(*cmd++)!=toupper(*test++)) return 0;
  if (*cmd!=' ') return 0;
  *params=cmd+1;
  return 1;
  }


char do_script(int section)
  {
  int pos;
  int num;
  int str_siz=str_count(script);
  char *command;

  for(pos=0;pos<str_siz;pos++) if (script[pos]!=NULL)
     {
     num=-1;
     sscanf(script[pos],"%d",&num);
     if (num==section && (command=strchr(script[pos],' '))!=NULL)
        {
        int i;
        command++;

        for(i=0;i<sizeof(commands)/4;i++)
           {
           char *p;
           if (command_match(command,commands[i],&p))
              if (calls[i](p))
                 {
                 char s[200];

                 sprintf(s,"Instal�tor nedok�z�l zpracovat ��dek '%s' - P��kaz '%s' vr�til chybu",command,commands[i]);
                 if (msg_box("Chyba v INF",'\x1',s,"Ignoruj","P�eru�it",NULL)==2) return 1;
                 }
           }
        }
     }
  return 0;
  }


char test_mscdex();
#pragma aux test_mscdex =\
     "mov  eax,1500h"\
     "mov  ebx,0h"\
     "int  2fh"\
     "cmp  al,0ffh"\
     "mov  al,0"\
     "jnz  nook"\
     "mov  al,cl"\
     "add  al,65"\
"nook:"\
  value[al] modify [eax ebx ecx]

int mscdex_ver();
#pragma aux mscdex_ver =\
     "mov  eax,150ch"\
     "int  2fh"\
     value [ebx] modify [eax]

char *get_cdrom()
  {
  static char s[80];
  char c;
  int v;

  c=test_mscdex();
  if (c)
     {
     v=mscdex_ver();
     sprintf(s," MSCDEX %d.%02d drive %c:",v/256,v%256,c);
     }
  else
     {
     strcpy(s,"<nen�>");
     }
  return s;
  }

char validate_path(char *path)
  {
  unsigned disk;
  unsigned cdisk,lastdrv;
  char *c,ll=0,tt=0,d;

  static char valid[]="$%'_@{}~`#()&-";

  if (path==NULL || path[0]==0 || path[1]!=':' || path[2]!='\\') return 0;
  disk=toupper(path[0]);
  if (disk<'A' || disk>'Z') return 0;
  _dos_getdrive(&cdisk);
  disk-='@';_dos_setdrive(disk,&lastdrv);
  _dos_getdrive(&lastdrv);
  disk=(disk==lastdrv);
  _dos_setdrive(cdisk,&lastdrv);
  if (!disk) return 0;
  c=path+2;ll=0;
  while (*c)
     {
     d=toupper(*c);
     if (d=='\\')
        if (ll) return 0;else ll=1,tt=0;
     else
        if (d>='A' && d<='Z' || d>='0' && d<='9' || strchr(valid,d)!=NULL || d>127) ll=0;
     else if (d=='.') if (tt) return 0;else tt=1;
     else return 0;
     c++;
     }
  if (ll) return 0;
  return 1;
  }

void clean_up()
  {
  int i,cnt;
  if (file_list==NULL) return;
  cnt=str_count(file_list);
  for(i=cnt-1;i>=0;i--) if (file_list[i]!=NULL)
     {
     if (remove(file_list[i])) rmdir(file_list[i]);
     }
  release_list(file_list);
  file_list=NULL;
  }

void purge_file_list()
  {
  if (file_list==NULL) return;
  release_list(file_list);
  file_list=NULL;
  }

char cascade_delete(char ignore_sav)
  {
  struct find_t f;
  int rc;

  rc=_dos_findfirst("*.*",_A_SUBDIR,&f);
  while (rc==0)
     {
     if (f.attrib & _A_SUBDIR)
        {
        if (f.name[0]!='.')
           {
           chdir(f.name);
           cascade_delete(ignore_sav);
           chdir("..");
           rmdir(f.name);
           }
        }
     else
        {
        char *c=strchr(f.name,'.');

        if (c!=NULL)
           {
           strupper(c);
           if (!ignore_sav || strncmp(c,".SAV",4)) remove(f.name);
           }
        else remove(f.name);
        }
     rc=_dos_findnext(&f);
     }
  return 0;
  }
