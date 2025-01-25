#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include "memman.h"

void help()
  {
  puts("Usage: Extract skeldal.ddl source.ext target.ext");
  exit(1);
  }

main(int argc,char **argv)
  {
  void *z;int32_t s;
  FILE *f;

  if (argc!=4) help();
  OPEN_LOG("");
  init_manager(argv[1],NULL);
  z=afile(argv[2],read_group(0),&s);
  if (z==NULL)
     {
     puts("File not found");
     return 1;
     }
  f=fopen(argv[3],"w");
  fwrite(z,1,s,f);
  fclose(f);
  puts("File successfuly expanded");
  return 0;
  }
