#include <skeldal_win.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "WAV.H"

int find_chunk(FILE *riff,char *name)
  {
  char chunk_name[4];
  int32_t next;

  fseek(riff,12,SEEK_SET);
  do
     {
     fread(chunk_name,1,4,riff);
     if (!strncmp(name,chunk_name,4)) return ftell(riff);
     if (fread(&next,1,4,riff)==0) return -1 ;
     if (fseek(riff,next,SEEK_CUR))return -1 ;
     }
  while (!feof(riff));
  return -1;
  }

int get_chunk_size(FILE *riff)
  {
  int32_t size;

  fread(&size,1,4,riff);
  fseek(riff,-4,SEEK_CUR);
  return(size);
  }

int read_chunk(FILE *riff,void *mem)
  {
  int32_t size,res;

  fread(&size,1,4,riff);
  res=fread(mem,1,size,riff);
  return res==size;
  }


