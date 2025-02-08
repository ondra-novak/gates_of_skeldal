#ifndef _WAV_H

#define _WAV_H

#include <stdint.h>

#define WAV_RIFF "RIFF"
#define WAV_WAVE "WAVE"
#define WAV_FMT  "fmt "
#define WAV_DATA "data"

typedef struct t_wave
  {
  unsigned short wav_mode,chans;
  int32_t freq,bps;
  }T_WAVE;

const char *find_chunk(const char *wav,char *name);
int get_chunk_size(const char *wav);
int read_chunk(const char *wav,void *mem);


#endif
