#ifndef _WAV_H

#define _WAV_H
#define WAV_RIFF "RIFF"
#define WAV_WAVE "WAVE"
#define WAV_FMT  "fmt "
#define WAV_DATA "data"

typedef struct t_wave
  {
  unsigned short wav_mode,chans;
  int32_t freq,bps;
  }T_WAVE;

char *find_chunk(char *wav,char *name);
int get_chunk_size(char *wav);
int read_chunk(char *wav,void *mem);


#endif
