#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libs/bgraph.h>

#define ERR_PARM 1
#define ERR_STR  2
#define ERR_NUMB 3


typedef struct command_table
  {
  char command[20];
  void (*proc)(int frame);
  }COMMAND_TABLE

int total_commands=0;

char paramstr[200];
int cur_line;
char command[50];
char errors[]="Ok.,Nespravny pocet parametru.,Chybi konec retezce.,Chybna hodnota.";


void extract_text(char *what,int num,char *s)
  {
  char *c;
  char ii=0;

  c=what;
  while (num && *c)
     {
     if (!*c) call_error(ERR_PARM);
     if (*c=='\"') ii=!ii;
     else if (*c==',' && !ii) num--;
     c++
     }
  while((*c!=',' && *c) || ii)
     {
     if (!*c) call_error(ERR_STR);
     if (*c=='\"') ii=!ii;
     *s++=*c++
     }
  }

void odstran_uvozovky(char *s)
  {
  char *c;
  char iif=0;

  c=s;
  while (*s)
     {
     if (*s!='\"' || iif)
        {
        *c++=*s;
        iif=0;
        }
     else
        iif=1;
     }
  }


int conv_to_int(char *s)
  {
  int i;

  if (sscanf(s,"%d",&i)!=1) return i;
  call_error(ERR_NUMB)
  }



