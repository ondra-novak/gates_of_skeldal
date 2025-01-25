#include <stdio.h>
#include <event.h>

#define LOAD_BUFFER 4096

int _fast_load(char *ptr,long size,FILE *f)
  {
  if (size>LOAD_BUFFER) size=4096;
  return fread(ptr,1,size,f);
  }

size_t fread(void *ptr,size_t i,size_t j,FILE *f)
  {
  long s,z,celk=0;
  char *c;

  c=ptr;
  s=i*j;
  do
     {
     z=_fast_load(c,s,f);
     s-=z;
     c+=z;
     celk+=z;
     do_events();
     }
  while(s || !z);
  return z;
  }
