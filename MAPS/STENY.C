#include <skeldal_win.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memman.h>
#include <types.h>
#include <mem.h>
#include <bgraph.h>
#include <event.h>
#include <devices.h>
#include <bmouse.h>
#include <gui.h>
#include <basicobj.h>
#include <strlists.h>
#include <strlite.h>
#include "steny.h"
#include "globals.h"

#define WLISTX 200
#define WLISTY 200

#define DELETED "*(deleted)"
#define EMPTY "EMPTY.PCX"
#define ANIM "*frame"

TSTR_LIST side_names=NULL;
TSTR_LIST floors=NULL;
TSTR_LIST ceils=NULL;
TSTR_LIST oblouky=NULL;
TSTR_LIST dlg_names=NULL;
TSTR_LIST weapons=NULL;
TSTR_LIST weapons_pos=NULL;

int *dlg_pgfs=NULL;

void read_side_list(char *filename,TSTR_LIST *outpt,int relative,int structlen)
  {
  char i,j;
  FILE *f;
  char s[200];


  if (*outpt!=NULL) release_list(*outpt);
  *outpt=create_list(1);
  str_replace(outpt,0,"<nic>");
  f=fopen(filename,"r");
  if (f==NULL) return;
  i=1;j=0;
  while (relative)
     {
     fscanf(f,"%s",&s);relative--;
     }
  do
     {
     if (fscanf(f,"%s",&s)==1 && (!j)) str_replace(outpt,i++,s);
     j++;
     if (j==structlen) j=0;
     }
  while (!feof(f));
  fclose(f);
  str_delfreelines(outpt);
  j=str_count(*outpt);
  for(i=0;i<j;i++) if ((*outpt)[i]!=NULL && (*outpt)[i][0]=='*') str_replace(outpt,i,NULL);
  }

void strlist_init(OBJREC *o,void **strtable)
  {
  o->userptr=(void *)getmem(16);
  memcpy(o->userptr,strtable,4);
  }


void read_dlg_list(char *filename,TSTR_LIST *outpt,int **nums)
  {
  FILE *f;
  char s[200];
  int i,j,k;

  f=fopen(filename,"r");
  i=0;
  if (*outpt!=NULL) release_list(*outpt);
  *outpt=create_list(1);
  str_replace(outpt,0,"       <nic>");
  if (f==NULL) return;
  do
     {
     if (fgets(s,199,f)==NULL) break;
     str_replace(outpt,i++,s);
     }
  while (!feof(f));
  fclose(f);
  if (*nums!=NULL) free(*nums);
  k=str_count(*outpt);
  *nums=(int *)getmem(sizeof(int)*k);
  for(j=0;j<i;j++) if ((*outpt)[j]!=NULL)
     sscanf((*outpt)[j],"%d %[^\0]",*nums+j,(*outpt)[j]);
  }

int pgf2name(int num)
  {
  int lcount;
  int i;

  lcount=str_count(dlg_names);
  for(i=0;i<lcount;i++) if (dlg_names[i]!=NULL && dlg_pgfs[i]==num) break;
  if (i==lcount) i=0;
  return i;
  }


void strlist_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  t_side_names *table,**t;
  int *d,i;

  d=(int *)o->data;
  t=(t_side_names **)o->userptr;
  table=*t;
  bar(x1,y1,x2,y2);
  for (i=0;i<*d;i++)
     if (!strcmp((*table)[i],"\\")) return;
  i=*d;
  while (y1+text_height((*table)[i])<y2 && strcmp((*table)[i],"\\"))
     {
     char s[100];
     strcpy(s,(*table)[i]);
     while (text_width(s)>o->xs)
        {
        char *t;

        t=strchr(s,'\0');
        *(--t)='\0';
        }
     position(x1,y1);
     outtext(s);
     y1+=text_height(*table);
     i++;
     }
  }
void strlist_event(EVENT_MSG *msg,OBJREC *o)
  {
  t_side_names *table,**t;
  int *d,i;

  d=(int *)o->data;
  t=(t_side_names **)o->userptr;
  table=*t;
  if (msg->msg==E_MOUSE)
     {
     MS_EVENT *ms;

     ms=get_mouse(msg);
     if (ms->event_type & 4)
        {
        int y1=0;

        i=*d;
        while (y1+text_height((*table)[i])<(ms->y-o->locy))
           {
           if (!strcmp((*table)[i],"\\")) return;
           y1+=text_height(*table);
           i++;
           }
        *d=i;
        terminate();
        }
     }
  if (msg->msg==E_CONTROL)
    {
    memcpy(o->userptr,msg->data,4);
    redraw_object(o);
    }
  }


void strlist(OBJREC *o)
  {
  o->runs[0]=strlist_init;
  o->runs[1]=strlist_draw;
  o->runs[2]=strlist_event;
  o->datasize=4;

  }

void strlist_scroll()
  {
  c_set_value(0,10,f_get_value(0,20));
  }

int strlist_count(char *s)
  {
  int i;

  i=0;
  while (strcmp(s,"\\")) i++,s+=T_LIST_SIZE;
  return i;
  }

TSTR_LIST build_static_list(char *c)
  {
  TSTR_LIST e;
  int i=0;

  e=create_list(10);
  while (c[0]!='\\')
     {
     str_replace(&e,i,c);
     c=strchr(c,0);
     c++;i++;
     }
  str_delfreelines(&e);
  return e;
  }


int string_list(char *c,int akt)
  {
  int oldid;
  int x1,y1;
  CTL3D *ctl;
  FC_TABLE cl;
  int vysl;

  cl[0]=0;cl[1]=0x610;
  ctl=def_border(3,WINCOLOR);
  oldid=o_aktual->id;
  x1=o_aktual->locx;
  y1=o_aktual->locy+o_aktual->ys;
  if (y1+WLISTY>desktop_y_size) y1=desktop_y_size-WLISTY;
  if (x1+WLISTX+4>SCR_WIDTH_X) x1=SCR_WIDTH_X-WLISTX-4;
  def_window(WLISTX,WLISTY,"Vyber");
  waktual->x=x1;
  waktual->y=y1;
  on_change(terminate);
  set_window_modal();
  define(19,5,20,WLISTX-30,WLISTY-20,0,listbox,c,RGB555(31,31,31),0);c_default(akt);
  on_change(terminate);
  define(20,3,42,17,WLISTY-64,1,scroll_bar_v,0,1,18,SCROLLBARCOL); 
  property(ctl,NULL,NULL,WINCOLOR);c_default(0);
  define(21,1,20,21,17,1,scroll_button,-1,0,"\x1e");
  property(NULL,icones,&cl,WINCOLOR);on_change(scroll_support);
  define(22,1,1,21,17,2,scroll_button,1,1,"\x1f");
  property(NULL,icones,&cl,WINCOLOR);on_change(scroll_support);
  redraw_window();
  send_message(E_GUI,19,E_CONTROL,2);
  redraw_window();
  goto_control(19);
  escape();
  if (o_aktual->id==19) vysl=f_get_value(0,19); else vysl=-1;
  close_window(waktual);
  return vysl;
  }

char find_nick(FILE *f,char *nick)
  {
  char rn[5];
  int i;

  do
     {
     i=fscanf(f,"%3s",rn);
     if (i==EOF) return 1;
     while (i!=EOF && i!='{') i=fgetc(f);
     if (!strcmp(rn,nick)) return 0;
     do
       {
       while (i!=EOF && i>=' ') i=fgetc(f);
       while (i!=EOF && i<=' ') i=fgetc(f);
       }
     while (i!=EOF && i!='}');
     }
  while (i!=EOF);
  return 1;
  }

void read_side_script_one(char *filename,char *nick,TSTR_LIST *outpt,int relative,int structlen)
  {
  char i,j;
  FILE *f;
  char s[200],end=0;

  if (*outpt!=NULL) release_list(*outpt);
  *outpt=create_list(1);
  str_replace(outpt,0,"<nic>");
  f=fopen(filename,"r");
  if (f==NULL) return;
  if (find_nick(f,nick))
     {
     fclose(f);
     return;
     }
  i=1;j=0;
  while (relative && !end)
     {
     fscanf(f,"%s",s);relative--;
     if (s[0]=='}') end=1;
     }
  if (!end)
  do
     {
     if (fscanf(f,"%s",s)==1 && (!j) && s[0]!='}') str_replace(outpt,i++,s);
     if (s[0]=='}') end=1;
     j++;
     if (j==structlen) j=0;
     }
  while (!feof(f) && !end);
  fclose(f);
  str_delfreelines(outpt);
  j=str_count(*outpt);
  for(i=0;i<j;i++) if ((*outpt)[i]!=NULL && (*outpt)[i][0]=='*') str_replace(outpt,i,NULL);
  }

void read_full_side_script(char *filename)
  {
  read_side_script_one(filename,NSID,&side_names,0,4);
  read_side_script_one(filename,NFLR,&floors,0,2);
  read_side_script_one(filename,NCEI,&ceils,0,2);
  read_side_script_one(filename,NOBL,&oblouky,0,3);
  }

void read_spec_procs()
  {
  read_side_script_one("specproc.lst","WAL",&wall_procs,0,1);
  read_side_script_one("specproc.lst","MOB",&mob_procs,0,1);
  }

static TSTR_LIST sid[4]={NULL};
static TSTR_LIST flr[2]={NULL};
static TSTR_LIST cei[2]={NULL};
static TSTR_LIST obl[3]={NULL};


static void scan_section_script(FILE *f,int count,TSTR_LIST *ls)
  {
  char s[200];
  int i;
  int j;

  for(i=0;i<count;i++) ls[i]=create_list(256);
  i=0;j=0;
  do
    {
    fscanf(f,"%s",s);
    if (s[0]=='}') break;
    str_replace(ls+i,j,s);
    i++;if (i>=count) i=0,j++;
    }
  while(1);
  }


void load_side_script(char *filename)
  {
  FILE *f;
  int i;

  if (sid[0]!=NULL) for(i=0;i<4;i++) release_list(sid[i]);
  if (flr[0]!=NULL) for(i=0;i<2;i++) release_list(flr[i]);
  if (cei[0]!=NULL) for(i=0;i<2;i++) release_list(cei[i]);
  if (obl[0]!=NULL) for(i=0;i<3;i++) release_list(obl[i]);
  f=script_name==NULL?NULL:fopen(filename,"r");
  if (f!=NULL)
    {
    fseek(f,0,SEEK_SET);if (!find_nick(f,NSID)) scan_section_script(f,4,sid);
    fseek(f,0,SEEK_SET);if (!find_nick(f,NFLR)) scan_section_script(f,2,flr);
    fseek(f,0,SEEK_SET);if (!find_nick(f,NCEI)) scan_section_script(f,2,cei);
    fseek(f,0,SEEK_SET);if (!find_nick(f,NOBL)) scan_section_script(f,3,obl);
    fclose(f);
    }
  }

static int add_to_script(TSTR_LIST *ls,char *name,int frames,int count,int max)
  {
  int cnt;
  int i,a,f,j;
  char *c,*d;

  if (ls[0]==NULL)
    {
    for(i=0;i<count;i++) ls[i]=create_list(256);
    f=0;
    }
  else
    {
    cnt=str_count(ls[0]);
    i=0,a=0;f=-1;
    while (a<frames)
       {
       if (i>=cnt || ls[0][i]==NULL || !strcmp(ls[0][i],DELETED)) a++,f=i;
       else a=0;
       i++;
       }
    }
  if (f+frames>max) return -1;
  c=alloca(strlen(name)+20);
  if (frames>1) sprintf(c,"%s(%d)",name,frames);else strcpy(c,name);
  for (d=c;*d;d++) if (*d<=' ') *d='_';
  str_replace(ls,f,c);for(i=1;i<count;i++) str_replace(ls+i,f,EMPTY);
  for (j=1;j<frames;j++)
    {
    str_replace(ls,f+j,ANIM);for(i=1;i<count;i++) str_replace(ls+i,f+j,EMPTY);
    }
  return f;
  }


int change_side(int script,int side,int pos,int frame,char *new)
  {
  TSTR_LIST ls;

  side+=frame;
  if (pos==0) return -1;
  switch (script)
    {
    case 0: ls=sid[0];if (ls==NULL) return -2;
            if (pos & 1) str_replace(sid+1,side,new);
            if (pos & 2) str_replace(sid+2,side,new);
            if (pos & 4) str_replace(sid+3,side,new);
            break;
    case 1: ls=flr[0];if (ls==NULL) return -2;
            if (pos!=1) return -1;
            str_replace(flr+1,side,new);
            break;
    case 2: ls=cei[0];if (ls==NULL) return -2;
            if (pos!=1) return -1;
            str_replace(cei+1,side,new);
            break;
    case 3: ls=obl[0];if (ls==NULL) return -2;
            if (pos & 1) return -1;
            if (pos & 2) str_replace(obl+1,side,new);
            if (pos & 4) str_replace(obl+2,side,new);
            break;
    default: return -3;
    }
  side++;
  if (side>=str_count(ls) || ls[side]==NULL || strcmp(ls[side],ANIM)) return 0;
  return 1;
  }

static int delete_side_st(TSTR_LIST *ls,int pos,int count)
  {
  int cnt,i;

  if (ls[0]==NULL) return -1;
  cnt=str_count(ls[0]);
  do
    {
    for(i=0;i<count;i++) str_replace(ls+i,pos,i==0?DELETED:EMPTY);
    pos++;
    }
  while (pos<cnt && ls[0][pos]!=NULL && !strcmp(ls[0][pos],ANIM));
  cnt=str_count(ls[0]);
  while (cnt>0 && (ls[0][cnt-1]==NULL || !strcmp(ls[0][cnt-1],DELETED)))
    {
    cnt--;
    str_replace(ls,cnt,NULL);
    }
  return 0;
  }

int delete_side(int list,int pos)
  {
  switch (list)
    {
    case 0: return delete_side_st(sid,pos,4);break;
    case 1: return delete_side_st(flr,pos,2);break;
    case 2: return delete_side_st(cei,pos,2);break;
    case 3: return delete_side_st(obl,pos,3);break;
    default: return -1;
    }
  }

int add_side(int list,char *name,int frames)
  {
  switch (list)
    {
    case 0: return add_to_script(sid,name,frames,4,256);break;
    case 1: return add_to_script(flr,name,frames,2,256);break;
    case 2: return add_to_script(cei,name,frames,2,256);break;
    case 3: return add_to_script(obl,name,frames,3,16);break;
    default: return -1;
    }
  }

char *get_side_name(int list,int pos,int field)
  {
  char **res=NULL;
  switch (list)
    {
    case 0: res=sid[field];break;
    case 1: res=flr[field];break;
    case 2: res=cei[field];break;
    case 3: res=obl[field];break;
    default: return NULL;
    }
  if (res==NULL) return NULL;
  return res[pos];
  }



void save_section_script(FILE *f,TSTR_LIST *ls,int count)
  {
  int i,j,cnt;

  cnt=str_count(ls[0]);
  for (j=0;j<cnt;j++)
    if (ls[0][j]!=NULL)
      for (i=0;i<count;i++)
      {
      fprintf(f,"%s",ls[i][j]);
      if (i+1==count) fputc('\n',f);else fputc(' ',f);
      }
  }

void save_side_script(char *filename)
  {
  FILE *f;
  int i;

  f=fopen(filename,"w");
  if (sid[0]!=NULL){fputs(NSID"\n{\n",f);save_section_script(f,sid,4);fputs("}\n\n",f);}
  if (flr[0]!=NULL){fputs(NFLR"\n{\n",f);save_section_script(f,flr,2);fputs("}\n\n",f);}
  if (cei[0]!=NULL){fputs(NCEI"\n{\n",f);save_section_script(f,cei,2);fputs("}\n\n",f);}
  if (obl[0]!=NULL){fputs(NOBL"\n{\n",f);save_section_script(f,obl,3);fputs("}\n\n",f);}
  fclose(f);
  read_full_side_script(filename);
  for(i=0;i<4;i++) release_list(sid[i]),sid[i]=NULL;
  for(i=0;i<2;i++) release_list(flr[i]),flr[i]=NULL;
  for(i=0;i<2;i++) release_list(cei[i]),cei[i]=NULL;
  for(i=0;i<3;i++) release_list(obl[i]),obl[i]=NULL;
  }

void discharge_side_script()
  {
  int i;
  for(i=0;i<4;i++) release_list(sid[i]),sid[i]=NULL;
  for(i=0;i<2;i++) release_list(flr[i]),flr[i]=NULL;
  for(i=0;i<2;i++) release_list(cei[i]),cei[i]=NULL;
  for(i=0;i<3;i++) release_list(obl[i]),obl[i]=NULL;
  }
