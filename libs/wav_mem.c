#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "wav_mem.h"

const char *find_chunk(const char *wav,char *name) //TODO improve
  {
  int32_t next;

  wav+=12;
  do
     {
     if (!strncmp(name,wav,4)) return wav+4;
     wav+=4;
     memcpy(&next,wav,4);
     wav+=next+4;
     }
  while (1);
  }

int get_chunk_size(const char *wav)
  {
  int32_t size;

  memcpy(&size,wav,4);
  return(size);
  }

int read_chunk(const char *wav,void *mem)
  {

  wav+=4;
  memcpy(mem,wav,get_chunk_size(wav-4));
  return 0;
  }


