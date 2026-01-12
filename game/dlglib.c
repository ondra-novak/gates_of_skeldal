/*

 definice formatu-

  :num  - odstavec
  #text

 # asterix = muze obsahovat tyto znaky
  . = normal text (vypisuje se)
  _ = popis (zapisuje se do LOGu)
  " = dialog (vypisuje se do LOGu a tiskne se)
  ? = vyber.. Nasleduje . nebo _ nebo '
  * = prikaz
  ; = komentar
 */


#include <libs/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libs/strlite.h>
#include <libs/memman.h>
#include <libs/bgraph.h>

FILE *dlg;
int32_t *odstavce=NULL;
int pocet;

int selptr=0;

char get_article()
  {
  int c;

  while ((c=fgetc(dlg))==32);
  return (char)c;
  }

#define new_line() while (fgetc(dlg)!='\n');


int count_pargh()
  {
  char c;
  int i=0;

  fseek(dlg,0,SEEK_SET);
  c=get_article();
  while (c!=0xff)
     {
     if (c==':') i++;
     if (c!='\n') new_line();
     c=get_article();
     }
  return i;
  }

void read_pargh()
  {
  int s,i;
  int32_t *d;
  char c;
  if (odstavce!=NULL) free(odstavce);
  odstavce=NULL;
  s=(pocet=i=count_pargh())*sizeof(int32_t)*2;
  if (s==0) return;
  odstavce=getmem(s);
  d=odstavce;
  fseek(dlg,0,SEEK_SET);
  c=get_article();
  while (c!=0xff && i)
     {
     if (c==':')
        {
        fscanf(dlg,"%d",&s);
        *d++=s;
        *d++=ftell(dlg);
        i--;
        }
     if (c!='\n') new_line();
     c=get_article();
     }
  }

void dlg_error(char *chyba)
  {
  closemode();
  printf("Error in dialoge: %s\n",chyba);
  exit(1);
  }

int32_t *najdi_odstavec(int odstavec)
  {
  int32_t *d;
  int i;


  d=odstavce;
  for(i=0;i<pocet && *d!=odstavec;i++) d+=2;
  if (i==pocet)
     {
     char s[50];
     sprintf(s,"Can't find paragraph num %d",odstavec);
     dlg_error(s);
     }
  return d;
  }

char jdi_na_odstavec(int odstavec)
  {
  int32_t *l,m;
  char c;

  l=najdi_odstavec(odstavec);
  m=l[1] & 0xffffff;
  c=l[1]>>24;
  fseek(dlg,m,SEEK_SET);
  return c;
  }

void set_flags(int n,int32_t maskand,int32_t maskor)
  {
  int32_t *l;
  l=najdi_odstavec(n);
  l[1]&=(maskand<<24)+0xffffff;
  l[1]|=maskor<<24;
  }

int param(char *c)
  {
  int i;
  sscanf(c,"%d",&i);
  return i;
  }

int nparam(int n,char *c)
  {
  char *d;

  if (n)
     {
     d=c;
     while (n--) d=(char *)strchr(d+1,',');
     }
     else d=c-1;
  if (d==NULL) return -1;else return param(d+1);
  }

void proved_goto(int num)
  {
   char c;
   int32_t l;
   l=ftell(dlg);
   c=jdi_na_odstavec(num);
        while (c & 1)
           {
           int i,j;

           j=fscanf(dlg,"%d",&i);
           if (j) c=jdi_na_odstavec(i); else
              {
              c=0;
              fseek(dlg,l,SEEK_SET);
              }
           }
  }
void proved_d(char *code,char *text)
  {
  static int mode=0;
  static char *save_text;

  do
  {
  if (mode==0)
  {
  *code=get_article();
  switch (*code)
     {
     case ';':
     case ':':*code=0xff;new_line();break;
     case '.':*code=1;break;
     case '_':*code=2;break;
     case '"':*code=3;break;
     case '?':*code=4;selptr++;break;
     case '*':*code=0;break;
     default :*code=0xff;break;
     }
  if (*code!=0xff)fscanf(dlg,"%[^\n]",text);
  if (*code==0)
     {
     strupper(text);
     if (!strncmp(text,"GOTO",4))
        {
        int n;
        sscanf(text+4,"%d",&n);
        proved_goto(n);
        }
     if (!strncmp(text,"JUMP",4))
        {
        int n;
        sscanf(text+4,"%d",&n);
        jdi_na_odstavec(n);
        }
     else if (!strncmp(text,"DISABLE",7)) set_flags(param(text+7),0xff,1);
     else if (!strncmp(text,"ENABLE",6)) set_flags(param(text+7),0xfe,0);
     else if (!strncmp(text,"CHOICE",6))
        {
        *text=selptr;
        selptr=0;
        *code=5;
        mode=1;
        save_text=getmem(strlen(text)-6+1);
        strcpy(save_text,text+6);
        }
     else if (!strncmp(text,"STOP",4)) *code=7;
     else if (!strncmp(text,"MENU",4)) selptr=param(text+4);
     }
  else if (*code==4)
     {
     int32_t *l=najdi_odstavec(param(text));
     char c,*d;

     c=l[1]>>24;
     if (c & 1) *code=6;
     d=text;while (*d>='0' && *d<='9') d++;
     strcpy(text,d);
     }
  }
  else if (mode==1)
     {
     int i=text[1],j;

     *code=5;
     j=nparam(i,save_text);
     if (j==-1) return;
     free(save_text);
     proved_goto(j);
     set_flags(j,0xff,0x1);
     mode=0;
     *code=0;
     }
  }
  while (*code==0);
  }

main()
  {
  char code,text[300];
  dlg=fopen_icase("test.txt","r");
  read_pargh();
  proved_goto(1);
  proved_d(&code,text);
  while (code!=7)
     {
     switch (code)
        {
        case 1:printf("%s\n",text);break;
        case 2:
        case 3:printf("%s\n",text);break;
        case 4:printf("  %c.%s\n",selptr+64,text);break;
        case 5:text[1]=toupper(getche())-65;putchar('\n');break;
        }
     proved_d(&code,text);
     }
  }


