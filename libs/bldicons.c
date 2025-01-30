#include <stdio.h>
#include <stdlib.h>

#include "memman.h"
#include "pcx.h"

FILE *src,*trg,*scr;
#define IKNNAME "IKONY%02d.LIB"
#define IKONSINLIB 17
#define PCXSIZE (2+2+2+512+45*55)
int icount=6;
char szBuff[65536];

void Error(char *text,char *param)
  {
  printf(text,param);
  puts("");
  exit(1);
  }

void ReadScript(char *script)
  {
  int i;
  int j=0;
  char *pcx;
  unsigned short *wpcx;
  scr=fopen_icase(script,"rt");
  if (scr==NULL) Error("Nemohu otevrit soubor: %s",script);
  i=fscanf(scr,"%s",szBuff);
  while (i!=EOF)
    {
    if (open_pcx(szBuff,A_8BIT,&pcx)==-1) Error("Nemohu pracovat se souborem: %s",szBuff);
    wpcx=(unsigned short *)pcx;
    if (wpcx[0]!=45 && wpcx[1]!=55) Error("Ikona musi mit rozmety 45x55. Soubor: %s",szBuff);
    if (j==0)
      {
      sprintf(szBuff,IKNNAME,icount++);
      printf("Sestavuji soubor %s\n",szBuff);
      trg=fopen_icase(szBuff,"wb");
      if (trg==NULL) Error("Nelze zapisovat do souboru %s",szBuff);
      }
    if (!fwrite(pcx,PCXSIZE,1,trg)) Error("Chyba nastala pri zapisu do souboru %s",szBuff);
    free(pcx);
    j++;
    if (j==18)
      {
      fclose(trg);
      j=0;
      }
    i=fscanf(scr,"%s",szBuff);
    }
  if (j!=0)
    {
    void *p;
    int s;
    p=malloc(s=(18-j)*PCXSIZE);
    memset(p,0,s);
    fwrite(p,s,1,trg);
    }
  fcloseall();
  }

void Help()
  {
  Error("Pouziti: \n"
        "\n"
        "BLDICONS <script>\n"
        "\n"
        "Parametrem programu je jmeno souboru, ktery obsahuje seznam vsech ikon\n"
        "(tj. soubory *.pcx). Ikony musi mit rozmery 45x55.\n"
        "Vystupem programu jsou soubory typu *.lib, ktere je nutne presunout do\n"
        "spravneho adresare.\n"
        ,NULL);
  }

void main(int argc,char **argv)
  {
  if (argc==1) Help();
  puts("**Pracuji**");
  ReadScript(argv[1]);
  puts("**Hotovo**");
  }
