#ifndef _WAV_H

#define _WAV_H
#define WAV_RIFF "RIFF"
#define WAV_WAVE "WAVE"
#define WAV_FMT  "fmt "
#define WAV_DATA "data"

typedef struct t_wave
  {
  unsigned short wav_mode,chans;
  long freq,bps;
  }T_WAVE;

int find_chunk(FILE *riff,char *name); //-1 neuspech, jinak pozice
int get_chunk_size(FILE *riff);       //velikost
int read_chunk(FILE *riff,void *mem); // 1 neuspech


#endif
