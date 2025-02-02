#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>

#include <libs/memman.h>
#include <platform/sound.h>
#include <libs/wav_mem.h>
#include <libs/event.h>
#include "globals.h"
#include <math.h>
#include <libs/strlite.h>

#include <string.h>
#include <unistd.h>
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
  const TMA_SOUND *data;              //4
  short xpos,ypos,side;         //10
  word volume,sample_block;            //14
  word sector;
  }SND_INFO;

static short chan_state[CHANNELS];
static short track_state[TRACKS];
short sample_volume=255;

typedef struct sound_side_map_dir_t {
    uint32_t distance;
    int32_t visit_counter;
} TSOUND_SIDE_MAP_DIR;

typedef struct sound_side_map_t {
    TSOUND_SIDE_MAP_DIR parts[4];
} TSOUND_SIDE_MAP;

static TSOUND_SIDE_MAP *current_sound_sector_map = NULL;
static size_t current_sound_sector_map_size = 0;
static int32_t current_sound_sector_map_counter = 0;

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
     aunlock(playings[channel].sample_block);
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

int set_channel_volume_from_sector(int channel,
                                    int sound_source_sector,
                                    int sound_source_side,
                                   int listener_sector,
                                   int listener_direction,
                                   int volume) {
    const int maxvolume = 32768;
    volume = 255*volume/100;
    int left = maxvolume;
    int right = maxvolume;
    if (sound_source_sector > mapsize) return 0;
    if (sound_source_sector <0 || sound_source_sector == listener_sector) {
        if (sound_source_side >= 0) {
            int ddf = (sound_source_side + 4 - listener_direction) & 3;
            switch (ddf) {
                default:
                case 0:
                case 2: left = right = maxvolume;break;
                case 1: left = maxvolume/4; right = maxvolume;break;
                case 3: left = maxvolume; right = maxvolume/4;break;
            }
        }
    } else {

        const TSOUND_SIDE_MAP *p = current_sound_sector_map+sound_source_sector;
        left = 0;
        right = 0;
        for (int i = 0; i < 4; ++i) {
            if (p->parts[i].visit_counter == current_sound_sector_map_counter) {
                int dist = p->parts[i].distance;
                if (!dist) return 1; //error
                int inc = maxvolume/(dist*dist);
                switch(i) {
                    default:
                    case 2:
                    case 0: left += inc; right += inc; break;
                    case 1: right += inc;break;
                    case 3: left += inc;break;

                }
            }
        }

    }
    left = (left * volume) >> 8;
    right = (right * volume) >> 8;
    if (left > maxvolume) left = maxvolume;
    if (right > maxvolume) right = maxvolume;
    if (left == 0  && right == 0) return 0;

    set_channel_volume(channel,left,right);
    return 1;

}

/*int calcul_volume(int chan,int x,int y,int side,int volume)
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
*/
const void *wav_load(const void *p, int32_t *s)
  {
  const char *sr;
  char *tg;
  void *tgr;
  struct t_wave x[3];

  sr=p;
  sr=find_chunk(sr,WAV_FMT);
  read_chunk(sr,&x);
  sr=p;
  sr=find_chunk(sr,WAV_DATA);
  *s=get_chunk_size(sr);
  tgr=tg=getmem(*s+sizeof(struct t_wave)+4);
  memcpy(tgr,x,sizeof(struct t_wave));
  tg+=sizeof(struct t_wave);
  *(int *)tg=*s;
  tg+=4;
  read_chunk(sr,tg);

  *s+=sizeof(struct t_wave)+4;
  return tgr;
  }

void play_effekt(int x,int y,int xd,int yd,int sector,int side,const TMA_SOUND *p)
  {
  int chan;
  int blockid;
  SND_INFO *track;
  const char *s;

  if (!sound_enabled) return;
  chan=find_free_channel(p->soundid);
  release_channel(chan);
  track=&tracks[p->soundid];
  track->data=p;
  track->xpos=xd;
  track->ypos=yd;
  track->side=side;
  track->sector = sector;
  track_state[p->soundid]=-1;
  if (p->bit16 & 0x8)
     {
     int vol=SND_EFF_MAXVOL*p->volume/100;
     if (rnd(100)>50) set_channel_volume(chan,rnd(vol),vol);
     else set_channel_volume(chan,vol,rnd(vol));
     }
  else {
      if (!set_channel_volume_from_sector(chan, sector, side, viewsector, viewdir, p->volume))
          return;
  }

    blockid = find_handle(p->filename, wav_load);
    if (blockid == -1) {
        def_handle(end_ptr, p->filename, wav_load, SR_ZVUKY);
        blockid = end_ptr++;
    }

      alock(blockid);
      s=ablock(blockid);
      s+=p->offset+sizeof(struct t_wave)+4;
      play_sample(chan,s,p->end_loop-p->offset,p->start_loop-p->offset,p->freq,1+(p->bit16 & 1));
      playings[chan].data=p;
      playings[chan].xpos=xd;
      playings[chan].ypos=yd;
      playings[chan].sector = sector;
      playings[chan].side=side;
      playings[chan].volume=p->volume;
      playings[chan].sample_block=blockid;
      chan_state[chan]=p->soundid;
      track_state[p->soundid]=chan;
  }

typedef struct _sound_map_queue_t {
    int from_sector;
    int to_sector;
} TSOUND_MAP_QUEUE;

static void build_dungeon_sound_map_in_dir(int sector, int side, int position, int32_t counter) {
    if (map_sectors[sector].step_next[side] == 0
               || !(map_sides[sector * 4 +side].flags & SD_TRANSPARENT)) return;

    TSOUND_MAP_QUEUE queue[128];
    int qbeg = 0;
    int qend = 0;
    queue[qend].from_sector = sector;
    queue[qend].to_sector = map_sectors[sector].step_next[side];
    ++qend;
    while (qbeg != qend) {
        const TSOUND_MAP_QUEUE *item = queue+(qbeg % countof(queue));
        int cursect = item->to_sector;
        int fromsect = item->from_sector;
        ++qbeg;
        if (current_sound_sector_map[cursect].parts[position].visit_counter != counter) {
            current_sound_sector_map[cursect].parts[position].visit_counter = counter;
            int d = current_sound_sector_map[cursect].parts[position].distance =
                    current_sound_sector_map[fromsect].parts[position].distance + 1;
            if (d<32) {
                for (int i = 0; i < 4; ++i) {
                    int nx = map_sectors[cursect].step_next[i];
                    if (nx && (map_sides[cursect * 4 +i].flags & SD_TRANSPARENT)) {
                        if (current_sound_sector_map[nx].parts[position].visit_counter != counter) {
                            TSOUND_MAP_QUEUE *t = queue+(qend % countof(queue));
                            ++qend;
                            t->from_sector = cursect;
                            t->to_sector = nx;
                        }
                    }
                }
            }
        }
    }

}

static void build_dungeon_sound_map(int sector, int side) {
    if (mapsize != (int)current_sound_sector_map_size) {
        free(current_sound_sector_map);
        current_sound_sector_map = NewArr(TSOUND_SIDE_MAP, mapsize);
        current_sound_sector_map_size = mapsize;
    }
    int32_t counter = ++current_sound_sector_map_counter;
    if (sector > mapsize) return;
    for (int i = 0; i < 4; ++i) {
        current_sound_sector_map[sector].parts[i].distance = 0;
        current_sound_sector_map[sector].parts[i].visit_counter = counter;
    }
    build_dungeon_sound_map_in_dir(sector, side, 0, counter);
    build_dungeon_sound_map_in_dir(sector, (side+1)&3, 1, counter);
    build_dungeon_sound_map_in_dir(sector, (side+2)&3, 2, counter);
    build_dungeon_sound_map_in_dir(sector, (side+3)&3, 3, counter);
}


void recalc_volumes(int sector,int side)
  {
  int i;
  int newx,newy;//,layer;

  if (sector>=mapsize) return;

  side;
  SEND_LOG("(SOUND) %s","Recalculating volumes");
  build_dungeon_sound_map(sector, side);
  newx=map_coord[sector].x;
  newy=map_coord[sector].y;
//  layer=map_coord[sector].layer;
  for(i=0;i<CHANNELS;i++) {
      int volume = playings[i].volume;
     if (chan_state[i]>=0 && playings[i].side>=0)
        {
         set_channel_volume_from_sector(
                 i,
                 playings[i].sector,
                 playings[i].side,
                 sector,
                 side,
                 volume);
        //calcul_volume(i,newx-playings[i].xpos,newy-playings[i].ypos,/*side-*/playings[i].side,playings[i].volume);
        if (!get_channel_state(i)) release_channel(i);
        }
     else {
         int v = SND_EFF_MAXVOL*volume / 100;
         set_channel_volume(i, v, v);
     }
  }
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
           if (have_loop(tracks[i].data))
               play_effekt(newx,newy,tracks[i].xpos,tracks[i].ypos,tracks[i].sector,tracks[i].side,tracks[i].data);
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
  if (!playlist[0]) return;
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
  if (!playlist[0]) return;
  for (c=playlist;c!=NULL;c=strchr(c+1,' ')) i++;
  playlist_size=i-1;
  cur_playlist=create_list(i);
  j=0;
  for (c=playlist;c!=NULL;c=strchr(c+1,' '))
     {
     char *e;
     char d[MAX_FILESYSTEM_PATH+2]="!";
     strncat(d,c+j,MAX_FILESYSTEM_PATH);d[MAX_FILESYSTEM_PATH+1]=0;j=1;
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

const char * end_of_song_callback(void *ctx) {
    return get_next_music_from_playlist();
}
const char *get_next_music_from_playlist()
  {
  int i,step;
  if (cur_playlist==NULL) return NULL;
  if (!remain_play)
     for(i=0;cur_playlist[i]!=NULL;remain_play++,i++) cur_playlist[i][0]=32;
  if (play_list_mode==PL_RANDOM)
     step=rnd(playlist_size)+1;
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
  const char *d = build_pathname(2, gpathtable[SR_MUSIC], cur_playlist[i]+1);
  if (!check_file_exists(d)) {
     return NULL;
  }
  cur_playlist[i][0]=33;
  remain_play--;
  return d;
  }

void purge_playlist()
  {
  if (cur_playlist!=NULL)release_list(cur_playlist);
  cur_playlist=NULL;
  }

void play_sample_at_sector(int sample,int listener,int source,int track, char loop)
  {
  int xd,yd,chan;
  const char *s;
  struct t_wave *p;
  int siz;
	int oldtrack;

  if (!sound_enabled) return;
  if (map_coord[listener].layer!=map_coord[source].layer) return;
  xd=map_coord[source].x;
  yd=map_coord[source].y;
  chan=find_free_channel(track);
	oldtrack=track_state[track];
  if (!track || oldtrack==-1) release_channel(chan);
  if (!set_channel_volume_from_sector(chan, source, -1, listener, -1, 100)) return;
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
  playings[chan].sample_block=sample;
  playings[chan].sector=source;
  chan_state[chan]=track;
  track_state[track]=chan;
  }

void play_sample_at_channel(int sample,int channel,int vol)
  {
  const char *s;
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


void create_sound_table(char *template,int32_t size)
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
  const char *c,*s;
  int32_t pocet;
  int i=0;

  if (sound_table==NULL) sound_table=create_list(2);
  s=c=ablock(H_SOUND_DAT);
  memcpy(&pocet,s,sizeof(int32_t));c+=4;
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
  SEND_LOG("(SOUND) Killing sound tracks...");
  for (i=0;i<CHANNELS;i++) release_channel(i);
  for (i=0;i<32;i++) if (locks[i]!=0) aunlock(locks[i]);
  }

char test_playing(int track)
  {
  return track_state[track]!=-1;
  }

static int flute_channel=30;
static int flute_channel_offset = 0;

void start_play_flute(char note)
  {
  const void *q;
  const char *w;
  float realfrq;
  int vol=50;

  realfrq=16000*pow(2,note/12.0);
  if (check_snd_effect(SND_GFX))
     {
     q=ablock(H_FLETNA);
     w=q;w+=sizeof(struct t_wave)+4;
     vol*=SND_EFF_MAXVOL/100;
     int ch = flute_channel+flute_channel_offset;
     set_channel_volume(ch,vol,vol);
     play_sample(ch,w,0x1665,0xADE,(int)(realfrq+0.5),1);
     }
  else
     {
     //sound((unsigned short)(realfrq/30.53));
     }
  }

void stop_play_flute()
  {
  const void *q;
  const char *w;

  if (check_snd_effect(SND_GFX))
     {
     q=ablock(H_FLETNA);
     w=q;w+=sizeof(struct t_wave);
     int ch = flute_channel+flute_channel_offset;
     chan_break_ext(ch,w+4,*(int *)w);
     flute_channel_offset = (flute_channel_offset+1) & 7;
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
