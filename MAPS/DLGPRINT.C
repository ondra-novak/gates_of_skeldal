#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DIALOGY "DIALOGY.DAT"
void *dialog;
int count;
char *pc;
char *ending;
char all_print,print_it;
char print2;

#define SAVE_POSTS 20
static char sn_nums[SAVE_POSTS];
static char sn_nams[SAVE_POSTS][32];
static char sn_rods[SAVE_POSTS];

static void error(char *text)
  {
  fputs(text,stderr);
  exit(1);
  }


typedef struct t_paragraph
  {
  unsigned num:15;
  unsigned alt:15;
  unsigned visited:1;
  unsigned first:1;
  long position;
  }T_PARAGRAPH;



void Load_dialogs()
  {
  FILE *f;
  int size;

  f=fopen(DIALOGY,"rb");
  if (f==NULL)
    {
    fprintf(stderr,"Nemohu otevrit soubor %s",DIALOGY);
    exit(1);
    }
  fseek(f,0,SEEK_END);
  size=ftell(f);
  fseek(f,0,SEEK_SET);
  dialog=malloc(size);
  fread(dialog,1,size,f);
  fclose(f);
  memcpy(&count,dialog,4);
  ending=pc=dialog;
  pc+=sizeof(T_PARAGRAPH)*count+8;
  ending+=size;
  }

static char buff[65535];

static char *transfer_text(char *source,char *target)
  {
  char *orgn=source,*ot=target,c;
  int num;
  print2=print_it=0;
  while (*source)
     {
     if (*source=='%')
        {
        source++;
        switch(*source)
           {
           case '[':*target++='[';break;
           case ']':*target++=']';break;
           case 'a':*target++='\'';break;
           case 'p':
           case '%':*target++='%';break;
           case 'n':strcpy(target,sn_nams[0]);target+=strlen(sn_nams[0]);print2=print_it=1;break;
           default: num=0;while (isdigit(*source)) num=10*num+*source++-'0';
                    if (*source=='l')
                       {
                       sn_nums[0]=sn_nums[num];
                       strcpy(sn_nams[0],sn_nams[num]);
                       sn_rods[0]=sn_rods[num];
                       }
                    break;
           }
           source++;
        }
     else if (*source=='[')
        {
        source++;print2=print_it=1;
        num=sn_rods[0];
        while(num>0)
           {
           source=strchr(source,',');
           num--;
           if (source==NULL)
              {
              puts(orgn);
              error("Chybn� rod nebo mal� po�et tvar� od jednoho slova");
              }
           source++;
           }
        while (*source!=',' && *source!=']' && *source!=0) *target++=*source++;
        if (*source!=']')
           {
           source=strchr(source,']');
           if (source==NULL)
              {
              puts(orgn);
              error("O�ek�v� se ']'");
              }
           }
        source++;
        }
     else c=*target++=*source++;
     }
  *target=0;
  //if (code_page==2)
    // prekodovat(ot);
  return target;
  }

void read_program()
  {
  while(pc<ending)
    {
    if (*pc++!=1) pc+=2;
      else
      {
      transfer_text(pc,buff);
      if (print2!=print_it) abort();
      if (print_it || all_print=='V')
        {
        printf("   ");
        puts(buff);
        if (print_it) puts("<print>");
        }
      pc=strchr(pc,0)+1;
      }
    }
  }

void read_names()
  {
  char fm;
  int i,j,k;

  do
    {
    fprintf(stderr,"Budes zad�vat mu�sk� jm�na, nebo �ensk�: (M/Z)");gets(buff);
    fm=toupper(buff[0]);
    }
  while (fm!='M'&& fm!='Z') ;
  fprintf(stderr,"Nyn� je t�eba zadat n�kolik jmen, posledn� odklepni ENTREM\n");
  i=0;
  do
    {
    fprintf(stderr,"%d.jm�no:",i+1);gets(buff);
    if (buff[0]) strncpy(sn_nams[i],buff,31);
    i++;
    }
  while(i<SAVE_POSTS && buff[0]);
  do
    {
    fprintf(stderr,"Bude� cht�t vyhledat v�echny texty (p�smeno V) nebo jen ty ve kter�ch se objevuj� jm�na postav (p�smeno P)");
    gets(buff);
    all_print=toupper(buff[0]);
    }
  while (all_print!='V' && all_print!='P') ;
  for(j=0;j<i;j++)
    {
    k=j%i;
    if (k!=j) strncpy(sn_nams[j],sn_nams[k],31);else puts(sn_nams[j]);
    sn_rods[j]=(fm=='Z');
    }
  }


void main()
  {
  Load_dialogs();
  read_names();
  read_program();
  }
