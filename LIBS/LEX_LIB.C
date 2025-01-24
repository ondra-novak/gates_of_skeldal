#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct lex_tree
    {
    char znak;
    char *data;
    struct lex_tree *left,*right,*middle;
    char params;
    }LEX_TREE;


LEX_TREE koren={0,NULL,NULL,NULL,NULL};

char cur_include[256];
char last_parms;

FILE *target_file;

void error_extend(char *file,int seek)
  {
  FILE *f;
  int c,i=0,ls=0,ls0=0,ls1=0,ls2=0;

  f=fopen(file,"r");
  if (f==NULL) return;
  c=getc(f);
  while (c!=EOF && ls2<seek)
     {
     if (c=='\n')
        {
        ls=ls0,ls0=ls1;ls1=ls2;ls2=ftell(f);
        i++;
        }
     c=getc(f);
     }
  printf("Chyba nastala v souboru '%s' na radce %d\n\n",file,i);
  fseek(f,ls,SEEK_SET);
  c=getc(f);
  i=0;
  while (c!=EOF && i<3)
     {
     putchar(c);
     if (c=='\n') i++;
     c=getc(f);
     }
  fclose(f);
  }

void error(char *text,int seek)
  {
  printf("CHYBA: %s \n",text);
  fcloseall();
  error_extend(cur_include,seek);
  exit(1);
  }

char *lex_find_string(char *text,LEX_TREE **z)
//najde retezec a vraci *z jako ukazatel na uzel ve strome + retezec
//ukazujici na posledni znak ktery se neshodoval (\0 retezec nasel)
  {
  LEX_TREE *p;

  p=&koren;
  do
     {
     *z=p;
     if (p->znak==*text)
       {
       if (!text[0]) break;
       text++;
       p=p->middle;
       }
     else if (p->znak>*text)  p=p->left;
     else p=p->right;
     }
  while (p!=NULL);
  return text;
  }


LEX_TREE *lex_add_string(char *text)
  {
  char *tp;
  LEX_TREE *z,*p,*q,*s;

  tp=lex_find_string(text,&z);
  if (*tp!=z->znak)
     {
     s=q=(LEX_TREE *)malloc(sizeof(LEX_TREE));
     memset(q,0,sizeof(LEX_TREE));
     q->znak=*tp;
     while (*tp)
       {
       tp++;
       p=(LEX_TREE *)malloc(sizeof(LEX_TREE));
       memset(p,0,sizeof(LEX_TREE));
       p->znak=*tp;
       q->middle=p;
       q=p;
       }
     if (s->znak>z->znak) z->right=s;
     else if (s->znak<z->znak) z->left=s;
     }
  return q;
  }



void lex_add_data(char *name,char *data,char parms)
  {
  LEX_TREE *z;
  char *d;

  z=lex_add_string(name);
  d=(char *)malloc(strlen(data)+1);
  strcpy(d,data);
  z->data=d;
  z->params=parms;
  }

char *lex_get_data(char *name)
  {
  LEX_TREE *z;
  name=lex_find_string(name,&z);
  if (*name) return NULL;
  last_parms=z->params;
  return z->data;
  }

char isnum(char *c)
  {
  if (*c=='-') c++;
  while (*c) if (!isdigit(*c++)) return 0;
  return 1;
  }

void open_include(char *name);


void xlat_text(FILE *source,FILE *target)
  {
  char s[256],c[256],*p;
  int i=0,z;
  int lastcom=0,count=0;
  char cm[256];

  while (i!=EOF)
  {
  while (((i=fgetc(source))<33 || i==',' || i=='(' || i==')') && i!=EOF ) if (i=='\n')
     if (count  && lastcom)
     {
     char c[255];
     sprintf(c,"Nespr vn˜ po‡et parametr– u p©¡kazu '%s'",cm);
     error(c,ftell(source));
     }
     else
        {
        count=1;
        lastcom=0;
        }
  if (i=='\'' || i=='"')
     {
     int j=i;
     i=fgetc(source);
     if (i=='\n') i=32;
     fputc('$',target);
        while (i!=j && i!=EOF)
           {
           fputc(i,target);
           i=fgetc(source);
           if (i=='\n') i=32;
           }
        fputc('\n',target);
        if (i==EOF) i=0;else i=1;
     fgetc(source);
     count--;
     continue;
     }
  else if (i==';')
     {
     while ((i=fgetc(source))!='\n' && i!=EOF);
     continue;
     }
  else if (i=='!')
     {
     ungetc(i,source);
     i=fscanf(source,"%[^\n]",s);
     if (i==1) fprintf(target,"%s\n",s);
     continue;
     }
  else
     {
     ungetc(i,source);
     i=fscanf(source,"%[^ ,()\n\x9]",s);
     }
  if(i!=EOF && i==1)
     {
     if (s[0]=='#')
        switch(s[1])
           {
           case 'i':fscanf(source,"%s",s);
                    open_include(s);
                    break;
           case 'd':fscanf(source,"%s %[^\n]",s,c);
                    lex_add_data(s,c,0);
                    break;
           case 'c':fscanf(source,"%s %s %d",s,c,&z);
                    lex_add_data(s,c,z);
                    break;
           case 'e':fprintf(target,"\n");
                    break;
           default: sprintf(c,"Neznamy prikaz '%s'",s);
                    error(c,ftell(source));
                    break;
           }
     else
        {
        if (isnum(s))
           {
           p=s;count--;
           }
           else
           {
           p=lex_get_data(s);
           if (last_parms && !count)
              {
              count=lastcom=last_parms;
              strcpy(cm,s);
              }
              else count--;
           }
        if (p!=NULL && i!=EOF) fprintf(target,"%s\n",p);
        else
           {
           sprintf(c,"Nedefinovane jmeno '%s'",s);
           error(c,ftell(source));
           }
        }
     }
  }
  }


void open_include(char *name)
  {
  FILE *inc;
  char *p;

  inc=fopen(name,"r");
  if (inc==NULL)
     {
     char s[256];

     sprintf(s,"Knihovna nenalezena: %s",name);
     error(s,0);
     }
  p=(char *)malloc(strlen(cur_include)+1);
  strcpy(p,cur_include);
  strcpy(cur_include,name);
  xlat_text(inc,target_file);
  fclose(inc);
  strcpy(cur_include,p);
  free(p);
  }

main(int argc,char *argv[])
  {
  if (argc!=3)
     {
     puts("Program vyzaduje dva parametry: LEX_LIB source_file target_file");
     exit(0);
     }
  target_file=fopen(argv[2],"w");
  open_include(argv[1]);
  fclose(target_file);
  }

