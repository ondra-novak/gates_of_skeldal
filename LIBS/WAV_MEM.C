#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "WAV_MEM.H"

char *find_chunk(char *wav,char *name)
  {
  long next;

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

int get_chunk_size(char *wav)
  {
  long size;

  memcpy(&size,wav,4);
  return(size);
  }

int read_chunk(char *wav,void *mem)
  {

  wav+=4;
  memcpy(mem,wav,get_chunk_size(wav-4));
  return 0;
  }


