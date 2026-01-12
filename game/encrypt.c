#include <libs/types.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

FILE *source,*target;

void encrypt_file(FILE *sr,FILE *tg)
  {
  int i,j,last=0;

  i=getc(sr);
  while (i!=EOF)
    {
    j=i-last;
    last=i;
    putc(j,tg);
    i=getc(sr);
    }
  }

void open_files(char *src,char *tgr)
  {
  if (tgr==NULL)
    {
    char *c,*d;
    tgr=alloca(strlen(src)+5);
    strcpy(tgr,src);
    c=strrchr(tgr,PATH_SEPARATOR_CHR);
    d=strrchr(tgr,'.');
    if (c>d) d=strchr(tgr,0);
    strcpy(d,".ENC");
    }
  source=fopen_icase(src,"rb");
  target=fopen_icase(tgr,"wb");
  }

void close_files()
  {
  fclose(source);
  fclose(target);
  }


char main(int argc,char **argv)
  {
  if (argc<2)
    {
    puts("Pouziti: ENCRYPT zdroj.ext [cil.ext] \n"
         "\n"
         "Pokud nezadas cil, doplni se zdroj s koncovkou .enc\n"
         "Nikdy nezadavej pouze cestu jako cil. Vzdycky uved jmeno!");
    return 0;
    }
  if (argc==2) open_files(argv[1],NULL);
  else open_files(argv[1],argv[2]);
  if (source==NULL)
    {puts("Nemuzu najit zdrojovy soubor\n");return 1;};
  if (target==NULL)
    {puts("Nemuzu otevrit cil pro zapis\n");return 1;};
  encrypt_file(source,target);
  close_files();
  puts("Ok.");
  return 0;
  }
