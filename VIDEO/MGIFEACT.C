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
#include <libs/mgifmem.h>
#include <libs/strlite.h>
#include <strlists.h>
#include "lzw.h"
#include "mgifeobj.h"
#include "mgifedit.h"
#include "mgifeact.h"
#include "mgifebld.h"


#define PRJ ".PRJ"

FRAME_DEFS_T *mgf_frames=NULL;
MGIF_HEADER_T mgf_header;
char *mgif_filename;

word preview_frame[320*180+3];
word check1;
word work_frame[320*180];
word check2;
word frame_shift;

TRACK_INFO_T *smp_prg=NULL;
int samples_total=0;
int last_unchanged_frame=0;


long total_frames=0;

void set_mgif(char *filename)
  {
  mgif_filename=NewArr(char,strlen(filename)+1);
  strcpy(mgif_filename,filename);
  }

#define error_exit(f,n) {fclose(f);return (n);}

int examine_mgif_file(char (*progress)(int perc))
  {
  FILE *f;
  MGIF_HEADER_T head;
  int r;
  int frame_size=0;
  char chunks;
  int chunk_size=0;
  char chunk_type;
  int frame,chunk;
  int track_size=-1;
  FRAME_DEFS_T *fd;
  FRAME_DEFS_T *fg;
  char fgp=0;

  frame_shift=0;
  f=fopen(mgif_filename,"rb");
  if (f==NULL) return EX_NOT_FOUND;
  r=fread(&head,1,sizeof(head),f);
  if (r!=sizeof(head)) error_exit(f,EX_READ_ERROR);
  memcpy(&mgf_header,&head,sizeof(head));
  total_frames=head.frames;
  mgf_frames=NewArr(FRAME_DEFS_T,total_frames);
  memset(mgf_frames,0,sizeof(FRAME_DEFS_T)*total_frames);
  fg=fd=mgf_frames;
  for(frame=0;frame<total_frames;frame++,fd++,fg+=fgp,frame_shift+=!fgp)
     {
     char sound=0;
     fd->frame_start=ftell(f);
     if (fread(&chunks,1,1,f)!=1) error_exit(f,EX_READ_ERROR);
     if (fread(&frame_size,1,3,f)!=3) error_exit(f,EX_READ_ERROR);
     for(chunk=0;chunk<chunks;chunk++)
        {
        if (fread(&chunk_type,1,1,f)!=1) error_exit(f,EX_READ_ERROR);
        if (fread(&chunk_size,1,3,f)!=3) error_exit(f,EX_READ_ERROR);
        switch(chunk_type)
           {
           case MGIF_LZW:
           case MGIF_DELTA:
           case MGIF_COPY: fg->display_start=ftell(f);
                           fg->displ_type=chunk_type;
                           fg->display_size=chunk_size;
                           fgp=1;
                           break;
           case MGIF_SOUND:fd->sound_start=ftell(f);
                           track_size=chunk_size;
                           sound=1;
                           fd->track_size=track_size;
                           break;
           case MGIF_PAL:  fg->pal_start=ftell(f);
                           break;

           }
        fseek(f,chunk_size,SEEK_CUR);
        }

     fd->changed=1;fd->vol_save=0;
     if (!sound) error_exit(f,EX_NO_SOUND);
     if (progress(frame*100/total_frames)) break;
     }
  fclose(f);
  return EX_NO_ERROR;
  }

int add_sample(char *sample)
  {
  char *d;

  d=NewArr(char,strlen(sample)+1);
  strcpy(d,sample);
  samples_total++;
  smp_prg=grealloc(smp_prg,samples_total*sizeof(TRACK_INFO_T));
  smp_prg[samples_total-1].sample_name=d;
  smp_prg[samples_total-1].loop_start=-1;
  smp_prg[samples_total-1].loop_end=-1;
  smp_prg[samples_total-1].user_name=NULL;
  smp_prg[samples_total-1].levy=NULL;
  smp_prg[samples_total-1].pravy=NULL;
  smp_prg[samples_total-1].starts=NULL;
  smp_prg[samples_total-1].starts_count=0;
  return samples_total-1;
  }

void remove_sample(int id)
  {
  TRACK_DATA_T *p,*q;
  if (id>=samples_total) return;
  p=smp_prg[id].levy;
  while(p!=NULL)
     {
     q=p->next;free(p);p=q;
     }
  p=smp_prg[id].pravy;
  while(p!=NULL)
     {
     q=p->next;free(p);p=q;
     }
  free(smp_prg[id].starts);
  free(smp_prg[id].sample_name);
  free(smp_prg[id].user_name);
  samples_total--;
  if (id!=samples_total)memcpy(smp_prg+id,smp_prg+id+1,(samples_total-id)*sizeof(TRACK_INFO_T));
  smp_prg=grealloc(smp_prg,samples_total*sizeof(TRACK_INFO_T));
  }

void build_sample_list(void *list)
  {
  TSTR_LIST ls;
  int id;

  memcpy(&ls,list,sizeof(void **));
  if (ls!=NULL) release_list(ls);
  ls=create_list(samples_total+1);
  for(id=0;id<samples_total;id++)
     {
     char *c;

     if (smp_prg[id].user_name!=NULL) c=smp_prg[id].user_name;
     else
        {
        c=strrchr(smp_prg[id].sample_name,'\\');
        if (c==NULL) c=smp_prg[id].sample_name;else c++;
        }
     str_add(&ls,c);
     }
  if (samples_total==0) str_add(&ls,"<��dn� zvuk>");
  memcpy(list,&ls,sizeof(void **));
  }


char get_vpoint(TRACK_DATA_T *track,int frame,TRACK_DATA_T *vp)
  {
  int fr;
  TRACK_DATA_T *p,*q;
  if (track==NULL)
     {
     vp->vpoint1=0;
     vp->vpoint2=0;
     vp->time=total_frames-frame;
     vp->next=NULL;
     return 1;
     }
  fr=0;
  q=p=track;
  while (p!=NULL && p->time+fr<=frame)
     {
     fr+=p->time;
     q=p;
     p=p->next;
     }
  if (p!=NULL)
     if (fr==frame)
        {
        memcpy(vp,p,sizeof(*vp));
        return 0;
        }
     else
        {
        int dif;
        int half;

        fr=frame-fr;
        half=p->time/2;
        dif=(p->vpoint2-p->vpoint1);
        vp->vpoint1=p->vpoint1+(dif*fr+half)/p->time;
        vp->vpoint2=p->vpoint2;
        vp->time=p->time-fr;
        vp->next=p->next;
        }
  else
     {
     vp->vpoint1=vp->vpoint2=q->vpoint2;
     vp->time=total_frames-frame;
     vp->next=NULL;
     }
  return 1;
  }

void add_vpoint(TRACK_DATA_T **track,int frame)
  {
  TRACK_DATA_T *p,*z;
  int fr,alltime;

  p=*track;
  if (p==NULL)
     {
     z=New(TRACK_DATA_T);
     get_vpoint(p,0,z);
     *track=z;
     z->next=NULL;
     add_vpoint(track,frame);
     return;
     }
  fr=0;
  while (p!=NULL && fr+p->time<=frame)
     {
     fr+=p->time;
     p=p->next;
     }
  if (p==NULL) crash("Frame out of range!");
  if (fr==frame) return;
  alltime=p->time;
  p->time=frame-fr;
  z=New(TRACK_DATA_T);
  z->vpoint1=p->vpoint1+((p->vpoint2-p->vpoint1)*p->time+alltime/2)/alltime;
  z->vpoint2=p->vpoint2;
  z->changed=1;p->changed=1;
  p->vpoint2=z->vpoint1;
  z->next=p->next;
  p->next=z;
  z->time=alltime-p->time;
  }

void change_vpoint_spacing(TRACK_DATA_T *track,int frame,int value)
  {
  TRACK_DATA_T *p,*q;
  int fr;

  p=track;
  if (track==NULL) return;
  while (p->next!=NULL) p=p->next;
  if (p->time<=value) value=p->time-1;
  if (!value) return;
  q=track;fr=0;
  while (q!=NULL && q->time+fr<=frame)
     {
     fr+=q->time;
     q=q->next;
     }
  if (q==NULL) return;
  if (fr+q->time+value<=frame) return;
  if (q->time+value<1) return;
  if (fr<last_unchanged_frame) last_unchanged_frame=fr;
  q->time+=value;
  p->time-=value;
  }

void set_vpoint(TRACK_DATA_T **track,int frame,int vp1,int vp2)
  {
  int fr=0;
  TRACK_DATA_T *p,*q;

  add_vpoint(track,frame);
  p=*track;q=NULL;
  while (p!=NULL && fr!=frame)
     {
     fr+=p->time;
     q=p;
     p=p->next;
     }
  p->changed=1;
  p->vpoint1=vp2;
  if (q!=NULL)
     {
     q->vpoint2=vp1;
     q->changed=1;
     }
  }

void add_restart(TRACK_INFO_T *ti,int frame)
  {
  int *p=ti->starts;
  int sc=ti->starts_count;
  int i;

  for(i=0;i<sc;i++) if (p[i]==frame) return;else if (p[i]>frame) break;
  p=grealloc(p,sizeof(int)*(sc+1));
  p[sc]=frame;
  for(i=sc-1;i>=0;i--)
     {
     if (p[i]>p[i+1])
        {
        p[i]^=p[i+1];
        p[i+1]^=p[i];
        p[i]^=p[i+1];
        }
     }
  ti->starts=p;
  ti->starts_count++;
  }

void delete_restart(TRACK_INFO_T *ti,int frame)
  {
  int i;
  int *p=ti->starts;
  int sc=ti->starts_count;

  for(i=0;i<sc;i++) if (p[i]>frame) return;else if (p[i]==frame) break;
  if (i==sc) return;
  memcpy(p+i,p+i+1,sizeof(int)*(sc-i-1));
  p=grealloc(p,sizeof(int)*(sc-1));
  ti->starts=p;
  ti->starts_count--;
  }

void delete_vpoint(TRACK_DATA_T **track,int frame)
  {
  int fr;
  TRACK_DATA_T *p,*q;

  fr=0;
  p=*track;
  if (p==NULL) return;
  q=NULL;
  while (p!=NULL && fr!=frame)
     {
     fr+=p->time;
     q=p;p=p->next;
     }
  if (p==NULL) return;
  if (q==NULL)
     {
     if (p->next!=NULL)
        {
        *track=p->next;
        p->next->vpoint1=p->vpoint1;
        p->next->time+=p->time;
        }
     else
        {
        *track=NULL;
        }
     free(p);
     }
  else
     {
     q->next=p->next;
     q->vpoint2=p->vpoint2;
     q->time+=p->time;
     free(p);
     }
  return;
  }


char join_two_frames(void *source,void *target);
#pragma aux join_two_frames parm[esi][edi]=\
     "mov  ecx,57600"\
     "xor  bl,bl"\
"lp1: lodsw"\
"     test [edi],0x8000"\
"     jz   sk1"\
"     mov  [edi],ax"\
"     or   bl,ah"\
"sk1: add  edi,2"\
"     dec  ecx"\
"     jnz  lp1"\
modify[ecx eax] value[bl];

void build_frame(int frame_num,int *exit)
  {
  void *data_buff;
  void *delta_buff;
  void *p;
  word palette[256];
  FILE *f;
  FRAME_DEFS_T *fd=mgf_frames+frame_num;
  char full=0;
  int counter=0;
  static last_frame=-1;
  char fast,flash;
  static zavora=0;

  if (last_frame==frame_num)
     {
     *exit=0;
     return;
     }
  if (frame_num>=total_frames) crash("frame_num>=total_frames");
  if (frame_num-last_frame!=1)
     {
     memset(preview_frame,0x80,sizeof(preview_frame));
     memset(work_frame,0x80,sizeof(work_frame));
     p=work_frame;fast=0;
     }
  else
     {
     p=preview_frame+3;
     last_frame=frame_num;
     fast=1;
     }
  preview_frame[0]=320;
  preview_frame[1]=180;
  preview_frame[2]=15;
  while (zavora==1) task_sleep(NULL);
  zavora=1;
  f=fopen(mgif_filename,"rb");
  data_buff=getmem(100000);
  delta_buff=getmem(100000);
  init_lzw_compressor(8);
  do
     {
     reinit_lzw();
     if (fd->pal_start)
        {
        fseek(f,fd->pal_start,SEEK_SET);
        fread(palette,1,sizeof(palette),f);
        }
     if (fd->display_start)
        {
        fseek(f,fd->display_start,SEEK_SET);
        fread(data_buff,1,fd->display_size,f);
        switch (fd->displ_type)
           {
           case MGIF_LZW:lzw_decode(data_buff,delta_buff);
                          show_full_lfb12e(delta_buff,p,palette);
                          break;
           case MGIF_DELTA:lzw_decode(data_buff,delta_buff);
                          show_delta_lfb12e(delta_buff,p,palette);
                          break;
           case MGIF_COPY:show_full_lfb12e(data_buff,p,palette);
                          break;
           }
        if (!fast) full=!(join_two_frames(work_frame,preview_frame+3) & 0x80);
        if (!((counter++) & 0xf) || full)
           {
           c_set_value(win_preview,10,0);
           c_set_value(win_preview,10,(int)&preview_frame);
           c_set_value(win_preview,5,(full||fast)?LABELCOLOR:(flash?0x7c00:0x3e0));
           flash=!flash;
           if (waktual->id!=win_preview)redraw_desktop();
           }
        if (!fast) task_sleep(NULL);
        if (task_info[curtask]) break;
        }
     fd--;
     }
  while (fd>=mgf_frames && !full && !fast);
  done_lzw_compressor();
  free(data_buff);
  free(delta_buff);
  fclose(f);
  if (full) last_frame=frame_num;
  zavora=0;
  *exit=0;
  }

extern char backsndbuff[BACK_BUFF_SIZE];
extern volatile long backsnd;
extern volatile long backstep;
extern volatile int  backfine;
extern long vals_save;

static stop_preview_flag=0;
static void *bufpos;


void preview_block(int start_frame,int x, int y)
  {
  word *scr;
  void *data_buff;
  char *dt;
  void *delta_buff;
  word *palette;
  FILE *f;
  FRAME_DEFS_T *fd=mgf_frames+start_frame;
  int i;
  char chunks;
  int fsize,shift;
  char chunk_type;
  int chunk_size;

  schovej_mysku();
  zobraz_mysku();
  stop_preview_flag=0;
  if (banking)scr=screen+640*y+x;else scr=(word *)lbuffer+640*y+x;
  memset(backsndbuff,0,sizeof(backsndbuff));
  i=total_frames-start_frame;
  bufpos=NULL;
  if (!i) return;
  start_mixing();
  f=fopen(mgif_filename,"rb");
  data_buff=getmem(100000);
  delta_buff=getmem(100000);
  shift=frame_shift;
  fseek(f,fd->frame_start,SEEK_SET);
  init_lzw_compressor(8);
  backsnd=0;backstep=0x10000;vals_save=fd->vol_save;
  do
     {
     char *gr,*s;char grtype;
     long ssize;
     fread(&fsize,1,4,f);chunks=fsize & 0xff;fsize>>=8;
     fread(data_buff,1,fsize,f);
     dt=data_buff;
     grtype=0xff;
     while (chunks--)
        {
        chunk_size=*(int *)dt;dt+=4;
        chunk_type=chunk_size&0xff;chunk_size>>=8;
        switch(chunk_type)
           {
           case MGIF_PAL:palette=(word *)dt;break;
           case MGIF_LZW:
           case MGIF_DELTA:reinit_lzw();lzw_decode(dt,delta_buff);grtype=chunk_type;break;
           case MGIF_COPY:gr=dt;grtype=chunk_type;break;
           case MGIF_SOUND:s=dt;ssize=chunk_size;break;
           }
        dt+=chunk_size;
        }
     do_events();
     while (test_next_frame(bufpos,ssize)) do_events();
     bufpos=sound_decompress(s,bufpos,ssize,&mgf_header.ampl_table);
     if (!shift)
        {
        switch(grtype)
           {
           case MGIF_LZW: show_full_interl_lfb130(delta_buff,scr,palette);break;
           case MGIF_DELTA:show_delta_interl_lfb130(delta_buff,scr,palette);break;
           case MGIF_COPY:show_full_interl_lfb130(gr,scr,palette);break;
           }
        if (banking) showview(x,y,320,180);
        }
     else shift--;
     fd++;
     }
  while (--i && !stop_preview_flag && !exit_wait);
  done_lzw_compressor();
  free(data_buff);
  free(delta_buff);
  fclose(f);
  stop_mixing();
  exit_wait=0;
  }

void stop_preview()
  {
  stop_preview_flag=1;
  }


void save_track(FILE *f,TRACK_DATA_T *t,char *id)
  {
  fprintf(f,"%s ",id);
  while (t!=NULL)
     {
     fprintf(f,"%d %d %d ",t->vpoint1,t->vpoint2,t->time);
     t=t->next;
     }
  fprintf(f,"\n");
  }

void save_starts(FILE *f,int *p,int pocet)
  {
  int i;

  fprintf(f,"%s %d ","START",pocet);
  for(i=0;i<pocet;i++) fprintf(f,"%d ",p[i]);
  fprintf(f,"\n");
  }

void save_project(char *project_name)
  {
  FILE *f;
  int i;
  TRACK_INFO_T *ti;

  f=fopen(project_name,"w");
  fprintf(f,"%d %d\n",samples_total,amplifikace);
  for(i=0,ti=smp_prg;i<samples_total;i++,ti++)
     {
     fprintf(f,"#%d",i);
     fprintf(f,"TRACK %s%s%d %d %d\n",ti->sample_name,ti->user_name==NULL?"":ti->user_name,
                 ti->loop_start,ti->loop_end,ti->starts_count);
     save_track(f,smp_prg[i].levy,"LEFT");
     save_track(f,smp_prg[i].pravy,"RIGHT");
     save_starts(f,smp_prg[i].starts,smp_prg[i].starts_count);
     if (smp_prg[i].muted) fputs("MUTED\n",f);
     }
  fclose(f);
  }

void *get_project_name(char *name)
  {
  char *c,*d;

  c=NewArr(char,strlen(name)+7);
  strcpy(c,name);
  d=strrchr(c,'\\');if (d==NULL) d=c;
  d=strchr(d,'.');if (d!=NULL) *d=0;
  strcat(c,PRJ);
  return c;
  }

static void skip_space(FILE *f)
  {
  int c;
  while  ((c=getc(f))==32);
  ungetc(c,f);
  }

static void skip_nline(FILE *f)
  {
  int c;
  while ((c=getc(f))!='\n' && c!=EOF);
  }

static void *load_track(FILE *f)
  {
  TRACK_DATA_T *p,*st;
  int vp1,vp2,tim,i;

  st=NULL;
  i=fscanf(f,"%d %d %d",&vp1,&vp2,&tim);
  while (i==3)
     {
     if (st==NULL) p=st=New(TRACK_DATA_T);
     else p=(p->next=New(TRACK_DATA_T));
     p->next=NULL;
     p->vpoint1=vp1;
     p->vpoint2=vp2;
     p->time=tim;
     p->changed=0;
     i=fscanf(f,"%d %d %d",&vp1,&vp2,&tim);
     }
  return st;
  }

static void load_starts(FILE *f,int **pole,int *pocet)
  {
  int i;
  fscanf(f,"%d",pocet);
  *pole=NewArr(int,*pocet);
  for(i=0;i<*pocet;i++) fscanf(f,"%d",*pole+i);
  }


int load_project(char *prj_name)
  {
  FILE *f;
  int p,i,j;
  TRACK_INFO_T *ti;

  f=fopen(prj_name,"r");
  if (f==NULL) return 0;
  fscanf(f,"%d %d",&p,&amplifikace);skip_nline(f);
  smp_prg=NewArr(TRACK_INFO_T,p);
  for(i=0;i<p;i++)
     {
     char s[20];
     skip_space(f);
     if (getc(f)!='#') {fclose(f);return -1;}
     fscanf(f,"%d",&j);
     ti=smp_prg+j;
     ti->levy=NULL;ti->pravy=NULL;ti->starts=NULL;
     do
        {
        skip_space(f);
        j=getc(f);ungetc(j,f);
        if (j=='#') break;
        if (j=='\n')
           {
           skip_nline(f);
           continue;
           }
        if (j==EOF) break;
        fscanf(f,"%s",s);skip_space(f);
        if (!strcmp(s,"TRACK"))
           {
           char s[256];
           ti->muted=0;
           fscanf(f,"%[^]",s);
           ti->sample_name=NewArr(char,strlen(s)+1);strcpy(ti->sample_name,s);
           getc(f);s[0]=0;
           fscanf(f,"%[^]",s);
           if (s[0])
              {
              ti->user_name=NewArr(char,strlen(s)+1);strcpy(ti->user_name,s);
              }
           else
              ti->user_name=NULL;
           getc(f);
           fscanf(f,"%d %d %d",&ti->loop_start,&ti->loop_end,&ti->starts_count);
           }
        else if (!strcmp(s,"LEFT")) ti->levy=load_track(f);
        else if (!strcmp(s,"RIGHT")) ti->pravy=load_track(f);
        else if (!strcmp(s,"START")) load_starts(f,&ti->starts,&ti->starts_count);
        else if (!strcmp(s,"MUTED")) ti->muted=1;
        else {fclose(f);return -1;}
        if (feof(f))
           {
           fclose(f);return -2;
           }
        skip_nline(f);
        }
     while (1);
     }
  samples_total=p;
  return 0;
  }

void set_changed_full_track(TRACK_INFO_T *tr)
  {
  set_changed_range(last_unchanged_frame,total_frames-1);
  set_changed_track(tr->levy);
  set_changed_track(tr->pravy);
  last_unchanged_frame=total_frames;
  }

static void do_vol_table_track(TRACK_DATA_T *tr,int indx,char action)
  {
  int i;
  int fr;
  int p;
  TRACK_DATA_T v;
  FRAME_DEFS_T *d;

  for(i=0,fr=0,d=mgf_frames;i<total_frames && tr!=NULL;i++,d++)
     {
     get_vpoint(tr,i-fr,&v);
     p=v.vpoint1+(v.vpoint2-v.vpoint1)/v.time;
     if (action) d->changed|=(d->last_vol[indx]!=p);
     d->last_vol[indx]=p;
     if (v.time==1)
        {
        fr+=tr->time;
        tr=tr->next;
        }
     }
  }

void read_vol_table(TRACK_INFO_T *tr)
  {
  do_vol_table_track(tr->levy,0,0);
  do_vol_table_track(tr->pravy,1,0);
  }

void compare_vol_table(TRACK_INFO_T *tr)
  {
  do_vol_table_track(tr->levy,0,1);
  do_vol_table_track(tr->pravy,1,1);
  }

void set_vol_table_nul(void)
  {
  int i;
  FRAME_DEFS_T *d;

  for(i=0,d=mgf_frames;i<total_frames;i++,d++)
     {
     d->last_vol[0]=0;
     d->last_vol[1]=0;
     }
  }

static long get_track_size(TRACK_INFO_T *tr)
  {
  long size1=0,size2=0;
  TRACK_DATA_T *l,*r;

  l=tr->levy;
  r=tr->pravy;
  while (l!=NULL) size1+=l->time,l=l->next;
  while (r!=NULL) size2+=r->time,r=r->next;
  if (size2>size1) return size2;else return size1;
  }

static void track_dopln_na_konci(TRACK_INFO_T *tr)
  {
  long size1=0,size2=0;
  TRACK_DATA_T *l,*r;

  l=tr->levy;
  r=tr->pravy;
  while (l->next!=NULL) size1+=l->time,l=l->next;
  while (r->next!=NULL) size2+=r->time,r=r->next;
  l->time=total_frames-size1;
  r->time=total_frames-size2;
  }

void warn_size_mistmach(void)
  {
  int size=total_frames;
  int sizeh=size;
  int i;

  for(i=0;i<samples_total;i++)
     {
     int s=get_track_size(smp_prg+i);
     if (s<size) size=s;
     if (s>sizeh) sizeh=s;
     }
  if (size<total_frames)
     {
     msg_box(PRG_HEADER,'\x1',"MGIFEDIT zjistil, �e film je del�i, ne� stopy "
        "ulo�en� v souboru PRJ. Program tyto stopy dopln� na konci. Ke spr�vn�mu "
        "um�st�n� pou�ij funkci GInsert","Ok",NULL);
     for(i=0;i<samples_total;i++) track_dopln_na_konci(smp_prg+i);
     }
  if (sizeh>total_frames)
     {
     msg_box(PRG_HEADER,'\x1',"MGIFEDIT zjistil,�e film je krat��, ne� stopy "
        "ulo�en� v souboru PRJ. Ke spr�vn�mu um�st�n� pou�ij funkci GDelete","Ok",NULL);
     }
  }


int get_inserted_frames(int frame)
  {
  int s;

  frame+=1;
  s=1;
  while (frame<total_frames)
     {
     char c=mgf_frames[frame].displ_type;
     if (c==MGIF_LZW || c==MGIF_COPY) return s;
     frame++;
     s++;
     }
  return s;
  }

int get_deleted_frames(void)
  {
  int size=total_frames;
  int sizeh=size;
  int i;
  for(i=0;i<samples_total;i++)
     {
     int s=get_track_size(smp_prg+i);
     if (s<size) size=s;
     if (s>sizeh) sizeh=s;
     }
  return sizeh-total_frames;
  }

void insert_global(int frame,int count)
  {
  int i;
  for(i=0;i<samples_total;i++)
     {
     change_vpoint_spacing(smp_prg[i].levy,frame,count);
     change_vpoint_spacing(smp_prg[i].pravy,frame,count);
     }
  }

void delete_global(int frame,int count)
  {
  int i,j;
  TRACK_DATA_T vl,vp;
  for(i=0;i<samples_total;i++)
     {
     get_vpoint(smp_prg[i].levy,frame,&vl);
     get_vpoint(smp_prg[i].pravy,frame,&vp);
     for(j=0;j<count;j++) if (frame+j>0)
      {
      delete_vpoint(&smp_prg[i].levy,frame+j);
      delete_vpoint(&smp_prg[i].pravy,frame+j);
      }
     change_vpoint_spacing(smp_prg[i].levy,frame,-count);
     change_vpoint_spacing(smp_prg[i].pravy,frame,-count);
     track_dopln_na_konci(smp_prg+i);
     set_vpoint(&smp_prg[i].levy,frame,vl.vpoint1,vl.vpoint1);
     set_vpoint(&smp_prg[i].pravy,frame,vp.vpoint1,vp.vpoint1);
     }
  }
