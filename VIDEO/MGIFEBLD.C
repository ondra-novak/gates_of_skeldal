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
#include <libs/wav.h>
#include <time.h>


static short **frame_table;
static char work_buff[100000];
static int last1=0;
static int last2=0;
static int last_time=0;

int frames_build=0;
int amplifikace=0;

typedef struct tsample
  {
  FILE *fs;
  int start_data;
  int length;
  int pos;
  long loop_start;
  long loop_end;
  char running;
  char bit16;
  char stereo;
  }TSAMPLE;

static TSAMPLE sample;

static void Create_table_16(short *ampl_table,int difftype)
  {
   int a,c,d,e;
   float b;

   d=-1;e=0;
   for(a=0;a<128;a++)
     {
     b=(a/128.0);
     switch (difftype)
        {
        case 1:b=(b*32768.0);break;
        case 2:b=(b*b*32768.0);break;
        case 3:b=(b*b*b*32768.0);break;
        case 4:b=(b*b*b*b*32768.0);break;
        case 5:b=(b*b*b*b*b*32768.0);break;
        }
     c=(int)b;
     if (c==d) e++;
     d=c;c+=e;
     ampl_table[128+a]=c;
     ampl_table[128-a]=-c;
     }
  }

static int find_index(short *ampl_table,int value)
  {
  int i;

  if (value==0) return 128;
  if (value>0)
     {
     for(i=128;i<256;i++)
        if (ampl_table[i]>value) return i-1;
     return 255;
     }
  else
     {
     for(i=128;i>=0;i--)
        if (ampl_table[i]<value) return i+1;
     return 1;
     }
  }

static void compress_buffer_stereo(short *ampl_table,short *source,int size)
  {
  int i;
  int l1=last1,l2=last2;
  int val;
  char indx;


  for(i=0;i<size;i++)
     {
     if (i & 1)
        {
        val=source[i];
        indx=find_index(ampl_table,val-l1);
        l1+=ampl_table[indx];
        if (l1>32768 || l1<-32768) crash("Building error (owerflow)");
        work_buff[i]=indx;
        }
     else
        {
        val=source[i];
        indx=find_index(ampl_table,val-l2);
        l2+=ampl_table[indx];
        if (l2>32768 || l2<-32768) crash("Building error (owerflow)");
        work_buff[i]=indx;
        }
     }
  last1=l1;last2=l2;
  }

static char save_buffer(FILE *f,int seekp,short *buffer,int size)
  {
  compress_buffer_stereo(mgf_header.ampl_table,buffer,size);
  fseek(f,seekp,SEEK_SET);
  return fwrite(work_buff,1,size,f)!=size;
  }

static char save_all()
  {
  int i,c=0;
  FRAME_DEFS_T *fd;
  FILE *f;

  f=fopen(mgif_filename,"rb+");
  if (f==NULL) return 1;
  fwrite(&mgf_header,1,sizeof(mgf_header),f);
  for(i=0,fd=mgf_frames;i<total_frames;i++,fd++) if (frame_table[i]!=NULL)
     {
     last1=(short)(fd->vol_save & 0xffff);
     last2=(short)(fd->vol_save>>16);
     if (save_buffer(f,fd->sound_start,frame_table[i],fd->track_size))
        {
        fclose(f);
        return 1;
        }
     c_set_value(0,10,(c++)*100/frames_build);
     do_events();if (exit_wait) break;
     fd->changed=0;
     free(frame_table[i]);frame_table[i]=NULL;
     if (i<total_frames-1) mgf_frames[i+1].vol_save=(last1 & 0xffff) | (last2<<16);
     }
  fclose(f);
  return 0;
  }


static void fill_frame_table(int from_frame)
  {
  int i;void *p;
  frames_build=0;
  if (frame_table==NULL) frame_table=NewArr(short *,total_frames);
  memset(frame_table,NULL,total_frames);
  for(i=from_frame;i<total_frames;i++) if (mgf_frames[i].changed)
     {
     int s=mgf_frames[i].track_size*2;
     p=_nmalloc(s);
     frame_table[i]=p;
     if (p==NULL) break;
     memset(p,0,s);
     frames_build++;
     }
  }

static char register_sample(char *sample_name,long loop_start,long loop_end)
  {
  FILE *f;
  T_WAVE *head;

  f=sample.fs=fopen(sample_name,"rb");
  if (f==NULL) return 1;
  if (find_chunk(f,WAV_FMT)!=-1)
     {
     head=(T_WAVE *)getmem(get_chunk_size(f));
     if (read_chunk(f,head) && (sample.start_data=find_chunk(f,WAV_DATA))!=-1)
        {
        sample.length=get_chunk_size(f);
        sample.fs=f;
        sample.running=0;
        sample.bit16=((head->bps/head->freq)/head->chans)==2;
        sample.stereo=(head->chans==2);
        sample.pos=0;
        if (loop_start==-1) loop_start=loop_end=sample.length;
        sample.loop_start=loop_start;
        sample.loop_end=loop_end;
        free(head);
        return 0;
        }
     free(head);
     }
  fclose(f);
  return 1;
  }


static void skip_frame(int frm_len) //v puvodni velikosti (16-bit stereo)
  {
  if (!sample.bit16) frm_len/=2;
  if (!sample.stereo) frm_len/=2;
  sample.pos+=frm_len;
  if (sample.loop_start<sample.loop_end)
     while (sample.pos>=sample.loop_end) sample.pos-=sample.loop_end-sample.loop_start;
  if (sample.pos>=sample.length) sample.running=0;
  }


static char build_sound(int frm_len,short *buffer,word vol1,word vol2)
  {
  short val1,val2;
  int rozsah,pos;
  char vol1l,vol2l,vol1r,vol2r;
  int ipos=ftell(sample.fs)-sample.start_data;

  vol1l=vol1 & 0xff;vol2l=vol2 & 0xff;
  vol1r=vol1>>8;vol2r=vol2>>8;
  rozsah=frm_len/4;
  pos=0;
  fseek(sample.fs,sample.pos+sample.start_data,SEEK_SET);
  while (frm_len>0)
     {
     short i,*j=&val1;

     if (ipos>sample.pos) fseek(sample.fs,sample.pos+sample.start_data,SEEK_SET);
     ipos=sample.pos;
     for(i=0;i<2;i++)
        {
        if (sample.bit16) fread(j,1,2,sample.fs);
        else
           {
           fread(j,1,1,sample.fs);*j<<=8;
           *j^=0x8000;
           }
        if (!sample.stereo)
           {
           val2=val1;break;
           }
        else
           j=&val2;
        }
     if (ferror(sample.fs)) return 1;
     val1=val1*(vol1l+(vol2l-vol1l)*pos/rozsah)/256;
     val1>>=amplifikace;
     val2=val2*(vol1r+(vol2r-vol1r)*pos/rozsah)/256;
     val2>>=amplifikace;
     skip_frame(4);
     if (*buffer+val1>32767) *buffer++=32767;
     else if (*buffer+val1<-32768) *buffer++=-32768;
     else *buffer+++=val1;
     if (*buffer+val2>32767) *buffer++=32767;
     else if (*buffer+val2<-32768) *buffer++=-32768;
     else *buffer+++=val2;
     pos++;
     frm_len-=4;
     if (!sample.running) break;
     }
  return 0;
  }


static char build_frame_sound(int frame,int sample)
  {
  TRACK_DATA_T z1l,z1r,z2l,z2r;

  if (frame_table[frame]==NULL)
     {
     skip_frame(mgf_frames[frame].track_size*2);
     }
  else
     {
     get_vpoint(smp_prg[sample].levy,frame,&z1l);
     get_vpoint(smp_prg[sample].pravy,frame,&z1r);
     get_vpoint(smp_prg[sample].levy,frame+1,&z2l);
     get_vpoint(smp_prg[sample].pravy,frame+1,&z2r);
     if (z1l.vpoint1 || z2l.vpoint1 || z1r.vpoint1 || z2r.vpoint1)
        return build_sound(mgf_frames[frame].track_size*2,
                 frame_table[frame],
                 (z1r.vpoint1<<8)+z1l.vpoint1,
                 (z2r.vpoint1<<8)+z2l.vpoint1);
     else
        skip_frame(mgf_frames[frame].track_size*2);
     }
  return 0;
  }

char build_one_sample(int smp)
  {
  int d=0;
  int i;
  int p,c,*pl;

  if (smp_prg[smp].muted) return 0;
  if (register_sample(smp_prg[smp].sample_name,
                  smp_prg[smp].loop_start,
                  smp_prg[smp].loop_end)) return 1;
  pl=smp_prg[smp].starts;
  c=smp_prg[smp].starts_count;
  for(i=0;i<total_frames;i++)
     {
     if (frame_table[i]!=NULL)
        {
        if ((d & 0x1f)==0) c_set_value(0,10,d*100/frames_build);
        d++;
        }
     if (exit_wait) break;
     for(p=0;p<c;p++)
        if (pl[p]==i)
           {
           sample.running=1;
           sample.pos=0;
           break;
           }
        else
           if (pl[i]>i) break;

     if (sample.running)
        {
        build_frame_sound(i,smp);
        do_events();
        }
     }
  fclose(sample.fs);
  return 0;
  }


static void destroy_all()
  {
  int i;

  for(i=0;i<total_frames;i++) if (frame_table[i]!=NULL)
     {
     free(frame_table[i]);frame_table[i]=NULL;
     }
  }

char build_solo(int smp)
  {
  Create_table_16(&mgf_header.ampl_table,3);
  fill_frame_table(0);
  if (build_one_sample(smp))
     {
     destroy_all();
     return 1;
     }
  save_all();
  return 0;
  }

static void error_message(int smp)
  {
  char *c;
  char *msg="Nastala chyba p�i pr�ci se stopou:";
  if (smp_prg[smp].user_name!=NULL)
     {
     concat(c,msg,smp_prg[smp].user_name);
     }
  else
    {
    concat(c,msg,smp_prg[smp].sample_name);
    }
  msg_box(PRG_HEADER,'\x1',msg,"Pokra�ovat",NULL);
  }

char build_all_samples()
  {
  int i;
  clock_t start_time,end_time;

  start_time=clock();
  Create_table_16(&mgf_header.ampl_table,3);
  fill_frame_table(0);
  for(i=0;i<samples_total;i++)
     {
     set_value(0,20,smp_prg[i].sample_name);
     if (build_one_sample(i))
        error_message(i);
     do_events();
     if (exit_wait)
        {
        exit_wait=0;
        destroy_all();
        return 0;
        }
     }
  set_value(0,20,"Ukl�d�m zvukovou stopu...");
  save_all();
  destroy_all();
  exit_wait=0;
  end_time=clock();
  last_time=(end_time-start_time)/CLOCKS_PER_SEC;
  return 0;
  }

void set_changed_range(int fr1,int fr2)
  {
  if (fr2>=total_frames) fr2=total_frames-1;
  for(;fr1<=fr2;fr1++) mgf_frames[fr1].changed=1;
  }

void set_changed_track(TRACK_DATA_T *ti)
  {
  int fr=0;
  while (ti!=NULL)
     {
     if (ti->changed) set_changed_range(fr,fr+ti->time);
     fr+=ti->time;
     ti->changed=0;
     ti=ti->next;
     }
  }

int get_building_time()
  {
  return last_time;
  }
