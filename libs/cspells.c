#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "types.h"

#define PGM_SIZE 200000
#define TAB_SIZE 512

typedef struct tkouzlo
  {
  word num,um,mge;
  word pc;
  short owner,accnum;     //accnum = akumulacni cislo, owner = kdo kouzlo seslal
  int start;
  short cil;
  char povaha;
  word backfire;
  word wait;   //wait - cekani pocet animaci
  word delay;  //delay - cekani pocet kol
  char traceon; //tracovani cile.
  char spellname[30];
  }TKOUZLO;


TKOUZLO kouzla_tab[TAB_SIZE];

char program[PGM_SIZE];
char *pgm;
FILE *source;
FILE *target;
int cur_spell;
char global_name[256];

void init(char *filename )
  {
  pgm=program;
  source=fopen_icase(filename,"r");
  cur_spell=0;
  if (source==NULL)
     {
     printf("INIT: Nemohu otevrit soubor %s \n",filename);
     exit(1);
     }
  }

void add_prog_command(int i)
  {
  *pgm++=i;
  }

void add_prog_word(int i)
  {
  *(short *)pgm=i;
  pgm+=2;
  }

void add_prog_string(char *c)
  {
  if (*c==32) c++;
  strcpy(pgm,c);
  pgm+=strlen(c)+1;
  }

int build_tables()
  {
  int num,i,num2;
  char s[256];

  do
     {
     i=fscanf(source,"%d",&num);
     if (i==1)
        {
        switch (num)
           {
           case 128:while (fgetc(source)!='\n');
                    fgetc(source);
                    i=fscanf(source,"%[^\n]",global_name);
                    strncpy(kouzla_tab[cur_spell].spellname,global_name,29);
                    printf("(%3d, 0x%05X) %s\n",cur_spell, kouzla_tab[cur_spell].start,global_name);
                    break;
           case 129:add_prog_command(0xff);
                    i=fscanf(source,"%d",&cur_spell);
                    if (i==1)
                       {
                       kouzla_tab[cur_spell].start=(pgm-program)+sizeof(kouzla_tab);
                       }
                    break;
           case 132:i=fscanf(source,"%d",&num);kouzla_tab[cur_spell].cil=num;break;
           case 133:i=fscanf(source,"%d",&num);kouzla_tab[cur_spell].um=num;break;
           case 134:i=fscanf(source,"%d",&num);kouzla_tab[cur_spell].mge=num;break;
           case 144:i=fscanf(source,"%d",&num);kouzla_tab[cur_spell].backfire=num;break;
           case 145:i=fscanf(source,"%d",&num);kouzla_tab[cur_spell].povaha=num;break;
           case 155:i=fscanf(source,"%d",&num);kouzla_tab[cur_spell].accnum=num;break;
           case 149:
           case 164:
           case 148:add_prog_command(num);i=fscanf(source,"%s",s);add_prog_string(s+1);break;
           case 192:add_prog_command(num);break;
           default: if (num==142) kouzla_tab[cur_spell].traceon|=1;
              add_prog_command(num);i=fscanf(source,"%d",&num2);add_prog_word(num2);
              if (num==146 && num2==12) kouzla_tab[cur_spell].traceon|=2;
              break;
           }
        }
  if (i==0)
     {
     printf("COMPILE: Chyba pri kompilaci kouzla '%s' \n",global_name);
     exit(0);
     }
     }
  while(i!=EOF);
  return pgm-program;
  }

void save_tab(char *name)
  {
  target=fopen_icase(name,"wb");
  if (target==NULL)
     {
     printf("SAVE: Chyba pri ukladani souboru %s \n",name);
     exit(0);
     }
  fwrite(kouzla_tab,1,sizeof(kouzla_tab),target);
  fwrite(program,1,pgm-program,target);
  fclose(target);
  }

const char *GetLexLibPath()
{
  static char c[MAX_PATH];
  char *z;
  GetModuleFileName(0,c,MAX_PATH);
  z=strrchr(c,PATH_SEPARATOR_CHR)+1;
  strcpy(z,"lex_lib.exe");
  return c;
}

main(int argc,char *argv[])
  {
  int codesize;
  char *z;
  printf("%d\n",sizeof(TKOUZLO));
  if (argc<2)
     {
     puts("Tento program vyzaduje jmeno souboru, ve kterem se nalezaji\n"
          "platne definice kouzel pro hru BRANY SKELDALU v 1.0");
     exit(0);
     }
  puts("");
  puts("Prob�h� kompilace:");
  puts("   Spou�t�m program LEX_LIB.EXE\n");
  putenv("DOS4G=QUIET");
  z=(char *)malloc(strlen(argv[1])+10);
  sprintf(z,"\"%s\"",argv[1]);
  if (spawnlp(P_WAIT,GetLexLibPath(),"lex_lib.exe",z,"temp.$$$",NULL))
     exit(1);
  free(z);
  if (errno)
     {
     puts("Nemohu spustit program lex_lib.exe");
     exit(1);
     }
  puts("Byla kompilov�na tato kouzla:");
  puts("��slo, za��tek, jmeno:");
  puts("======================");
  memset(kouzla_tab,0,sizeof(kouzla_tab));
  init("temp.$$$");
  codesize=build_tables();
  add_prog_command(0xff);
  fclose(source);
  save_tab("kouzla.dat");
  remove("temp.$$$");
  puts("Kompilace �sp��n�...");
  printf("D�lka k�du: %d (+%d)",codesize,sizeof(kouzla_tab));
  }




