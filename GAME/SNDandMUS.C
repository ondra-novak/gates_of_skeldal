#include <skeldal_win.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <memman.h>
#include <zvuk.h>
#include <wav_mem.h>
#include <event.h>
#include "globals.h"
#include <math.h>
//#include <i86.h>  //Sound and Nosound
#include <strlite.h>
#include <io.h>

#define PL_RANDOM 1
#define PL_FORWARD 2
#define PL_FIRST 3

#define CHANNELS 20
#define TRACKS 512

#define SND_EFF_MAXVOL 32000
#define SND_EFF_DESCENT 8000

#define have_loop(x) ((x)->start_loop!=(x)->end_loop)

typedef unsigned short SND_FIND_TABLE[2];
typedef struct snd_info
  {
  TMA_SOUND *data;              //4
  short xpos,ypos,side;         //10
  word volume,block;            //14
  }SND_INFO;

static short chan_state[CHANNELS];
static short track_state[TRACKS];
static short sample_volume=255;

//static struct t_wave wav_last_head;
//static int wav_last_size;
static int mute_task=-1;
static char sound_enabled=1;

SND_INFO tracks[TRACKS];
SND_INFO playings[CHANNELS];
static word locks[32];

TSTR_LIST cur_playlist=NULL;
TSTR_LIST sound_table=NULL;
int playlist_size;
int playing_track=0;
int remain_play=0;
int play_list_mode=PL_RANDOM;

void init_tracks()
  {
  memset(tracks,0,sizeof(tracks));
  memset(playings,0,sizeof(playings));
  memset(chan_state,0xff,sizeof(chan_state));
  memset(track_state,0xff,sizeof(track_state));
  memset(locks,0,sizeof(locks));
  }

static char last_beep_lev;

/*void pcspeak_uroven(char value,int time);
#pragma aux pcspeak_uroven parm[bh][ecx]=\
        "mov ah,last_beep_lev"\
    "lp2:add ah,bh"\
        "mov al,48h"\
        "jc lp1"\
        "mov al,4ah"\
    "lp1:out 61h,al"\
        "loop lp2"\
        "mov last_beep_lev,ah"\
   modify [eax]


static int get_pc_speed()
  {
  int ticks=0;
  int timer=get_timer_value();
  while (get_timer_value()-timer<50) pcspeak_uroven(127,1000),ticks+=1000;
  return ticks;
  }

void pc_speak_play_sample(char *sample,int size,char step,int freq)
  {
  static speed=0;
  int ticker;
  if (!speed) speed=get_pc_speed();
  _disable();
  ticker=speed/freq;
  sample+=step/2;
  while (size>0)
     {
     if (step==2)
        pcspeak_uroven(*sample ^ 0x80,ticker);
     else
        pcspeak_uroven(*sample,ticker);
     sample+=step;
     size-=step;
     }
  _enable();
  nosound();
  }

*/

int find_free_channel(int stamp)
  {
  int i,j;
  int minvol,left,right,mid;

  j=0;
  if (stamp) for(i=0;i<CHANNELS;i++) if (chan_state[i]==stamp) return i;
  minvol=0xffff;
  for(i=0;i<CHANNELS;i++)
     {
     if (!get_channel_state(i)) return i;
     get_channel_volume(i,&left,&right);
     mid=(left+right)/2;
     if (playings[i].side<0) mid*=2;
     if (mid<minvol)
        {
        minvol=mid;j=i;
        }
    }
  return j;
  }




void release_channel(int channel)
  {
  int i;

  i=chan_state[channel];
  if (i==-1) return;
  mute_channel(channel);
     {
     aunlock(playings[channel].block);
     chan_state[channel]=-1;
     track_state[i]=-1;
     }
  }

int calc_volume(int *x,int *y,int side)
  {
  int ds;

  side&=3;*x=-(*x);*y=-(*y);
  *x+=(side==1)*(*x>=0)-(side==3)*(*x<=0);
  *y+=(side==2)*(*y>=0)-(side==0)*(*y<=0);
  ds=abs(*x)+abs(*y);
  ds=SND_EFF_MAXVOL-(SND_EFF_DESCENT*8*ds)/(8+ds);
  return ds;
  }

int calcul_volume(int chan,int x,int y,int side,int volume)
  {
  int lv,rv;
  int ds,bal,i;

  if (side==-1) side=viewdir;
  side&=3;
  ds=calc_volume(&x,&y,side);
  if (ds<=0)
     {
     release_channel(chan);
     return -1;
     }
  for(i=0;i<viewdir;i++)
    {
    bal=x;
    x=y;
    y=-bal;
    }
  y=abs(y);
  if (abs(x)>y)
    if (x>0) bal=100-y*50/x;else bal=-100-y*50/x;
  else bal=50*x/y;
  ds=ds*volume/100;
  if (bal<0)
     {
     lv=ds*(100+bal)/100;rv=ds;
     }
  else
     {
     rv=ds*(100-bal)/100;lv=ds;
     }
  lv=(lv*sample_volume)>>8;
  rv=(rv*sample_volume)>>8;
  set_channel_volume(chan,lv,rv);
  return 0;
  }

void wav_load(void **p,long *s)
  {
  char *sr;
  long *d;
  char *c;
  char *tg;
  void *tgr;
  size_t siz;
  struct t_wave x[3];

  sr=*p;
  sr=find_chunk(sr,WAV_FMT);
  read_chunk(sr,&x);
  sr=*p;
  sr=find_chunk(sr,WAV_DATA);
  *s=get_chunk_size(sr);
  tgr=tg=getmem(*s+sizeof(struct t_wave)+4);
  memcpy(tgr,x,sizeof(struct t_wave));
  tg+=sizeof(struct t_wave);
  *(int *)tg=*s;
  tg+=4;
  read_chunk(sr,tg);
  free(*p);
  *p=tgr;
  siz=*s;
  *s+=sizeof(struct t_wave)+4;
/*  if (x[0].freq!=x[0].bps)
     {
     char s;

     siz>>=1;
     s=siz & 1;
     siz>>=1;
     d=tg;
     for(;siz--;d++) *d^=0x80008000;
     if (s) {c=(char *)d;c[1]^=0x80;}
     }
  else
     {
     char s;

     s=siz & 3;
     siz>>=2;
     d=(long *)tg;
     for(;siz--;d++) *d^=0x80808080;
     c=(char *)d;
     for(;s--;c++) *c^=0x80;
     }*/
  }

void play_effekt(int x,int y,int xd,int yd,int side,int sided,TMA_SOUND *p)
  {
  int chan;
  int blockid;
  SND_INFO *track;
  THANDLE_DATA *z;
  char *s;

  if (!sound_enabled) return;
  side;
  chan=find_free_channel(p->soundid);
  release_channel(chan);
  track=&tracks[p->soundid];
  track->data=p;
  track->xpos=xd;
  track->ypos=yd;
  track->side=sided;
  track_state[p->soundid]=-1;
  if (p->bit16 & 0x8)
     {
     int vol=SND_EFF_MAXVOL*p->volume/100;
     if (rnd(100)>50) set_channel_volume(chan,rnd(vol),vol);
     else set_channel_volume(chan,vol,rnd(vol));
     }
  else
     if (calcul_volume(chan,x-xd,y-yd,/*side-*/sided,p->volume)) return;
  if (p->filename[0]==1) memcpy(&blockid,&p->filename[1],4);
  else
     {
     blockid=find_handle(p->filename,wav_load);
     if (blockid==-1)
        {
        z=def_handle(end_ptr,p->filename,wav_load,SR_ZVUKY);
        blockid=end_ptr++;
        if (level_preload) apreload(blockid);
       }
     memcpy(&p->filename[1],&blockid,4);
     p->filename[0]=1;
     }
  alock(blockid);
  s=ablock(blockid);
  s+=p->offset+sizeof(struct t_wave)+4;
  play_sample(chan,s,p->end_loop-p->offset,p->start_loop-p->offset,p->freq,1+(p->bit16 & 1));
  playings[chan].data=p;
  playings[chan].xpos=xd;
  playings[chan].ypos=yd;
  playings[chan].side=sided;
  playings[chan].volume=p->volume;
  playings[chan].block=blockid;
  chan_state[chan]=p->soundid;
  track_state[p->soundid]=chan;
  }

void restore_sound_name(TMA_SOUND *p)
  {
  int blockid;
  THANDLE_DATA *h;

  if (p->filename[0]==1)
     {
      memcpy(&blockid,&p->filename[1],4);
     do
        {
        h=get_handle(blockid);
        if (h->status==BK_SAME_AS) blockid=h->seekpos;else blockid=-1;
        }
     while (blockid!=-1);
     strncpy(p->filename,h->src_file,12);
     }
  }

void restore_sound_names()
  {
  int i;

  for(i=0;i<mapsize*4;i++)
     if (macros[i]!=NULL)
        {
        int *r,mcsiz;
        TMULTI_ACTION *z;

        r=macros[i];
        while ((mcsiz=*r)!=0)
           {
           r++;
           z=(TMULTI_ACTION *)r;
           if (z->general.action==MA_SOUND) restore_sound_name(&z->sound);
           r=(int *)((char *)r+mcsiz);
           }
        }
  }

void recalc_volumes(int sector,int side)
  {
  int i;
  int newx,newy,layer;

  if (sector>=mapsize) return;

  side;
  SEND_LOG("(SOUND) %s","Recalculating volumes",0);
  newx=map_coord[sector].x;
  newy=map_coord[sector].y;
  layer=map_coord[sector].layer;
  for(i=0;i<CHANNELS;i++)
     if (chan_state[i]>=0 && playings[i].side>=0)
        {
        calcul_volume(i,newx-playings[i].xpos,newy-playings[i].ypos,/*side-*/playings[i].side,playings[i].volume);
        if (!get_channel_state(i)) release_channel(i);
        }
     else calcul_volume(i,0,0,-1,playings[i].volume);
  for(i=1;i<TRACKS;i++) if (track_state[i]<0 && tracks[i].data!=NULL)
     {
     if (tracks[i].side<0)
        {
        if (have_loop(tracks[i].data))
          play_effekt(0,0,0,0,-1,-1,tracks[i].data);
        }
     else
        {
       int x=newx-tracks[i].xpos, y=newy-tracks[i].ypos;
        if (calc_volume(&x,&y,tracks[i].side)>0)
           if (have_loop(tracks[i].data))play_effekt(newx,newy,tracks[i].xpos,tracks[i].ypos,side,tracks[i].side,tracks[i].data);
        }
     }
  mute_task=-1;
  }

void create_playlist(char *playlist)
  {
  char *c;
  char mode[20];
  char shift;
  int i=1,j;
  if (cur_playlist!=NULL) release_list(cur_playlist);
  cur_playlist=NULL;
  if (playlist==NULL) return;
  if (playlist=="") return;
  c=playlist;
  while (*c && *c==32) c++;
  sscanf(c,"%s",mode);
  strupr(mode);
  shift=1;
  if (!strcmp(mode,"RANDOM")) play_list_mode=PL_RANDOM;
  else if (!strcmp(mode,"FORWARD")) play_list_mode=PL_FORWARD;
  else if (!strcmp(mode,"FIRST")) play_list_mode=PL_FIRST;
  else shift=0;
  if (shift) c+=strlen(mode);else play_list_mode=PL_RANDOM;
  while (*c && *c==32) c++;
  playlist=c;
  if (playlist=="") return;
  for (c=playlist;c!=NULL;c=strchr(c+1,' ')) i++;
  playlist_size=i-1;
  cur_playlist=create_list(i);
  j=0;
  for (c=playlist;c!=NULL;c=strchr(c+1,' '))
     {
     char *e;
     char d[MAX_PATH+2]="!";
     strncat(d,c+j,MAX_PATH);d[MAX_PATH+1]=0;j=1;
     if ((e=strchr(d,32))!=NULL) *e=0;
     str_add(&cur_playlist,d);
     }
  if (play_list_mode==PL_FIRST)
     {
     cur_playlist[0][0]=32;
     remain_play=1;
     play_list_mode=PL_RANDOM;
     }
  else
     {
     remain_play=0;
     }
  playing_track=-1;
  }

void play_next_music(char **c)
  {
  int i,step;
  static char d[MAX_PATH];

  *c=NULL;
  if (cur_playlist==NULL) return;
  if (!remain_play)
     for(i=0;cur_playlist[i]!=NULL;remain_play++,i++) cur_playlist[i][0]=32;
  if (play_list_mode==PL_RANDOM)
     step=rand()*(playlist_size-1)/32768+1;
  else
     step=1;
  i=playing_track;
  do
     {
     i++;
     if (cur_playlist[i]==NULL) i=0;
     if (cur_playlist[i][0]==32) step--;
     }
  while (step);
  playing_track=i;
  sprintf_s(d,sizeof(d),"%s%s",pathtable[SR_MUSIC],cur_playlist[i]+1);
  if (_access(d,0) == -1)
      sprintf_s(d,sizeof(d),"%s%s",pathtable[SR_ORGMUSIC],cur_playlist[i]+1);
  cur_playlist[i][0]=33;
  remain_play--;
  *c=d;
  }

void purge_playlist()
  {
  if (cur_playlist!=NULL)release_list(cur_playlist);
  cur_playlist=NULL;
  }

void play_sample_at_sector(int sample,int sector1,int sector2,int track, char loop)
  {
  int x,y,xd,yd,chan;
  char *s;
  struct t_wave *p;
  int siz;
	int oldtrack;

  if (!sound_enabled) return;
  if (map_coord[sector1].layer!=map_coord[sector2].layer) return;
  x=map_coord[sector1].x;
  y=map_coord[sector1].y;
  xd=map_coord[sector2].x;
  yd=map_coord[sector2].y;
  chan=find_free_channel(track);
	oldtrack=track_state[track];
  if (!track || oldtrack==-1) release_channel(chan);
  if (calcul_volume(chan,x-xd,y-yd,viewdir,100)) return;
  if (!track || oldtrack==-1)
     {
     alock(sample);
     s=ablock(sample);
     p=(struct t_wave *)s;
     s+=sizeof(struct t_wave);
     siz=*(int *)s;s+=4;
     play_sample(chan,s,siz,loop?0:siz,p->freq,(p->freq!=p->bps?2:1));
     playings[chan].data=NULL;
     }
  playings[chan].xpos=xd;
  playings[chan].ypos=yd;
  playings[chan].side=viewdir;
  playings[chan].volume=100;
  playings[chan].block=sample;
  chan_state[chan]=track;
  track_state[track]=chan;
  }

void play_sample_at_channel(int sample,int channel,int vol)
  {
  char *s;
  struct t_wave *p;
  int siz;

  if (!sound_enabled) return;
  channel+=CHANNELS;
  vol*=SND_EFF_MAXVOL/100;
  set_channel_volume(channel,vol,vol);
  if (locks[channel]) aunlock(locks[channel]);
  alock(sample);
  locks[channel]=sample;
  s=ablock(sample);
  p=(struct t_wave *)s;
  s+=sizeof(struct t_wave);
  siz=*(int *)s;s+=4;
  play_sample(channel,s,siz,siz,p->freq,(p->freq!=p->bps?2:1));
  }


void create_sound_table(char *template,long size)
  {
  char *c,*s;
  int i=0;

  if (sound_table==NULL) sound_table=create_list(2);
  s=c=template;
  while (c-s<size)
     {
     if (c[0]!=0) str_replace(&sound_table,i,c);
     c=strchr(c,0)+1;
     i++;
     }
  }

void create_sound_table_old()
  {
  char *c,*s;
  long pocet;
  int i=0;

  if (sound_table==NULL) sound_table=create_list(2);
  s=c=ablock(H_SOUND_DAT);
  memcpy(&pocet,s,sizeof(long));c+=4;
  while (pocet--)
     {
     if (c[0]!=0) str_replace(&sound_table,i,c);
     c=strchr(c,0)+1;
     i++;
     }
  }


void stop_track(int track)
  {
  int chan;
  chan=track_state[track];
  if (chan==-1) return;
  chan_break_loop(chan);
  }

void stop_track_free(int track)
  {
  int chan;
  chan=track_state[track];
  if (chan==-1) return;
  chan_break_loop(chan);
  track_state[track]=-1;
  chan_state[chan]=0;
  }

void mute_all_tracks(char all)
  {
  int i;
  for(i=0;i<CHANNELS;i++)
     if (playings[i].side!=-1 || all) release_channel(i);
  mute_task=-1;
  SEND_LOG("(SOUND) %s (%d)","MUTE Tracks",all);
  }


void kill_all_sounds()
  {
  int i;
  SEND_LOG("(SOUND) Killing sound tracks...",0,0);
  for (i=0;i<CHANNELS;i++) release_channel(i);
  for (i=0;i<32;i++) if (locks[i]!=0) aunlock(locks[i]);
  }

char test_playing(int track)
  {
  return track_state[track]!=-1;
  }

static int flute_canal=30;

void start_play_flute(char note)
  {
  void *q;
  char *w;
  float realfrq;
  int vol=50;

  realfrq=16000*pow(2,note/12.0);
  if (check_snd_effect(SND_GFX))
     {
     q=ablock(H_FLETNA);
     w=q;w+=sizeof(struct t_wave)+4;
     vol*=SND_EFF_MAXVOL/100;
     set_channel_volume(flute_canal,vol,vol);
     play_sample(flute_canal,w,0x1665,0xADE,(int)(realfrq+0.5),1);
     }
  else
     {
     //sound((unsigned short)(realfrq/30.53));
     }
  }

void stop_play_flute()
  {
  void *q;
  char *w;

  if (check_snd_effect(SND_GFX))
     {
     q=ablock(H_FLETNA);
     w=q;w+=sizeof(struct t_wave);
     chan_break_ext(flute_canal,w+4,*(int *)w);
     flute_canal^=1;
     }
  else
     {
     //nosound();
     }
  }

char enable_sound(char enbl)
  {
  register char save;

  save=sound_enabled;
  sound_enabled=enbl;
  SEND_LOG("(SOUND) Sound status (en/dis) changed: new %d, old %d",enbl,save);
  return save;
  }
