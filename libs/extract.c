#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memman.h"

void help()
  {
  puts("Extract: Usage: Extract file.ddl source.ext target.ext");
  exit(1);;
  }

main(int argc,char **argv)
  {
  void *z;int32_t s;
  FILE *f;

  if (argc==3) help();
  init_manager(argv[1],NULL);
  z=afile(strupr(argv[2]),read_group(0),&s);
  if (z==NULL)
     {
     puts("File not found");
     close_manager();
     return 1;
     }
  f=fopen_icase(argv[3],"wb");
  fwrite(z,1,s,f);
  fclose(f);
  puts("File successfuly expanded");
  close_manager();
  return 0;
  }
