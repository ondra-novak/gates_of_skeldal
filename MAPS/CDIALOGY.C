#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <ctype.h>
#include <windows.h>

#define P_STRING 1
#define P_SHORT 2
#define P_VAR 3
#define TEMP_NAME1 "cdialogy.$$$"
#define SCRIPT "dialogy.scr"

typedef struct t_paragraph
  {
  unsigned num:15;
  unsigned alt:15;
  unsigned visited:1;
  unsigned first:1;
  long position;
  }T_PARAGRAPH;


char *pgf_name=NULL;

T_PARAGRAPH *pgf_list=NULL;
int pgf_count=0;
char *program;
int prog_size=0;
int prog_pos=0;
int last_pgf;
int sentencs=0;
int libs=0;

FILE *f;
FILE *mlist;

#define newline(f) {int c;while ((c=fgetc(f))!='\n' && c!=EOF);}
#define get_cislo(f,cislo) fscanf(f,"%d",cislo);newline(f)
#define get_command(f,cislo) fscanf(f,"%c%d",cislo,cislo);newline(f)

static int basic_num;
static int abs_num;
static int last_dialoge;
static int standard_jump;
static char sentence;
static char sub_mode,b_closed=1;

static int open_level=0;
static int iff_mode=0;
static char was_sub=0;
#define IF_JUMP 1
#define IF_NOT_JUMP 2
#define IF_ELSE 3

typedef struct if_record
   {
   int level;
   int pointer;
   }IF_RECORD;

int if_ptr=0;

#define IF_LEN 32

static IF_RECORD if_data[IF_LEN];

int init(char *c)
  {
  prog_size=163840;
  prog_pos=0;
  program=malloc(prog_size);
  f=fopen(c,"rt");
  if (f==NULL) return 1;
  return 0;
  }

void error(char *c)
  {
  printf("%s v odstavci %d (Dialog %d Sentence %d).",c,last_pgf,basic_num/128,last_pgf);
  exit(1);
  }

void resize(void **p,int newsize)
  {
  void *q;

  if (*p==NULL) q=malloc(newsize);else
     {
     q=malloc(newsize);
     if (q!=NULL)
        {
        memcpy(q,*p,_msize(*p));
        free(*p);
        }
     }
  if (q==NULL)
     {
     puts("Nedostatek pamØti");
     exit(1);
     }
  *p=q;
  }

void check_prog_space(int num)
  {
  if (prog_pos+num>prog_size-16)
     {
     prog_size+=163840;
     resize(&program,prog_size);
     }
  }

void add_byte(char byte)
  {
  check_prog_space(1);
  program[prog_pos++]=byte;
  }

void add_short(short i)
  {
  add_byte(P_SHORT);
  check_prog_space(2);
  *(short*)(program+prog_pos++)=i;
  prog_pos++;
  }

void add_var(short i)
  {
  add_byte(P_VAR);
  check_prog_space(2);
  *(short*)(program+prog_pos++)=i;
  prog_pos++;
  }

void copy_short(FILE *f)
  {
  int c;
  int i;
  c=fgetc(f);
  if (c=='&')
    {
    get_cislo(f,&i);
    add_var(i);
    }
  else
    {
    if (isdigit(c)) ungetc(c,f);
    get_cislo(f,&i);
    add_short(i);
    }
  }

void add_string(char *c)
  {
  add_byte(P_STRING);
  check_prog_space(strlen(c)+1);
  while (*c && *c!='\n') program[prog_pos++]=*c++;
  program[prog_pos++]=0;
  }

void add_pgf(T_PARAGRAPH *p)
  {
  resize(&pgf_list,sizeof(*p)*(pgf_count+1));
  memcpy(pgf_list+pgf_count,p,sizeof(*p));
  pgf_list[pgf_count].position=prog_pos;
  pgf_list[pgf_count].visited=0;
  pgf_list[pgf_count].first=0;
  if (pgf_name!=NULL)
     {
     fprintf(mlist,"%6d %s\n",pgf_list[pgf_count].num,pgf_name);
     free(pgf_name);
     pgf_name=NULL;
     }
  pgf_count++;
  }

void decode_pgf1(char *c) //c je text bez vykricniku
  {
  int pn,an,i;
  T_PARAGRAPH p;
  char relative=0;
  if (*c=='!')
     {
     relative=1;
     c++;
     }
  i=sscanf(c,"%d",&pn);
  if (relative) pn+=abs_num;else abs_num=pn;
  if (i!=1) error("Chybn‚ pou§it¡ znaku \"!\"");
  last_pgf=pn;
  c=strchr(c,',');
  if (c==NULL) an=pn;
  else
     {
     c++;
     i=sscanf(c,"%d",&an);
     if (i!=1) error("OŸek v  se Ÿ¡slo alternativn¡ho odstavce");
     }
  p.num=pn;
  p.alt=an;
  add_pgf(&p);
  }

void js_dialoge(FILE *f)
  {
  int parm1;
  T_PARAGRAPH pgf;

  if (!b_closed) error("Ocekava se '}' (close_block)");
  get_cislo(f,&parm1);
  abs_num=basic_num=parm1*128;
  pgf.num=basic_num;
  pgf.alt=basic_num;
  last_dialoge=pgf_count;
  last_pgf=0;
  add_pgf(&pgf);
  get_command(f,&parm1);
  if (parm1!=-2) error ("Ocekava se '{' (open_block)");
  sentence=0;
  b_closed=0;
  add_short(167);
  add_short(basic_num);
  }

void js_first(FILE *f)
  {
  int parm1;


  add_short(166);
  add_short(last_pgf);
  add_short(140);
  get_cislo(f,&parm1);
  add_short(parm1);
  }

void js_standard(FILE *f)
  {
  int parm1;

  get_cislo(f,&parm1);
  if (parm1>0) standard_jump=parm1;else standard_jump=32767+parm1-basic_num;
  }

void js_close_block()
  {
  if (open_level)
     {
     if (was_sub)
        if (sub_mode)
           add_short(165);
        else
           {
           add_short(139);
           add_short(standard_jump);
           }
     sub_mode=0;
     open_level--;
     return;
     }
  if (b_closed) error("Neocekavany znak '}' (close_block)");
  if (sentence && sub_mode) add_short(165);else
     {
     add_short(139);
     add_short(standard_jump);
     }
     b_closed=1;
  }

void js_sentence(FILE *f)
  {
  int par1,par2;
  T_PARAGRAPH pgf;

  if (!b_closed || open_level) error("Ocekava se '}' (close_block)");
  b_closed=0;
  get_cislo(f,&par1);
  get_cislo(f,&par2);
  sub_mode=par2;
  pgf.num=par1+basic_num;
  pgf.alt=par1+basic_num;
  add_pgf(&pgf);
  sentence=1;
  last_pgf=par1;
  get_command(f,&par1);
  if (par1!=-2) error ("Ocekava se '{' (open_block)");
  sentencs++;
  was_sub=0;
  }


void js_sub(FILE *f)
  {
  int par1;
  char *c;
  int d;

  c=malloc(65536);
  d=getc(f);while (d!=EOF && d!='$') d=getc(f);
  fgets(c,65536,f);
  if (sub_mode)
     {
     add_short(142);
     get_cislo(f,&par1);
     if (par1>0) add_short(par1);else add_short(32767+par1-basic_num);
     add_string(c);
     }
  else
     {
     add_short(148);
     add_string(c);
     get_cislo(f,&par1);
     if (par1>0) standard_jump=par1;else standard_jump=32767+par1-basic_num;
     }
  free(c);
  }

void js_close_sub()
  {
  was_sub=1;
  if (sub_mode)add_short(164); else add_short(144);
  }

void save_pgf_name(FILE *f)
  {
  long l;
  int i,s;
  l=ftell(f);
  for(s=0;(i=fgetc(f))!=EOF && i!='\n';s++);
  if (pgf_name!=NULL) free(pgf_name);
  pgf_name=(char *)malloc(s+2);
  fseek(f,l,SEEK_SET);
  fgets(pgf_name,s+1,f);
  fgetc(f);
  }

void push_level(int level,int pos)
  {
  if (if_ptr==IF_LEN) error("Priliz dlouha struktura if & else");
  if_data[if_ptr].level=level;
  if_data[if_ptr].pointer=pos;
  if_ptr++;
  }

void pop_and_fill(int pos,int level)
  {
  int i;
  int prog_sav;
  prog_sav=prog_pos;
  while (if_ptr && if_data[if_ptr-1].level==level)
     {
     if_ptr--;
     prog_pos=if_data[if_ptr].pointer;
     i=pos-if_data[if_ptr].pointer-3;
     if (i>32767 || i<-32767) error("Blok if/else je priliz dlouhy (delsi nez 32767 bytu)");
     add_short(i);
     }
  prog_pos=prog_sav;
  }

int get_last_level()
  {
  if (if_ptr) return if_data[if_ptr-1].level;
  else return -1;
  }

void add_if_jump()
  {
  char c=1;
  switch(iff_mode)
     {
     case IF_NOT_JUMP:add_short(169);break;
     case IF_JUMP:add_short(170);break;
     case IF_ELSE:c=0;break;
     }
  if (c)
     {
     push_level(open_level,prog_pos);
     add_short(0);
     }
  iff_mode=0;
  was_sub=0;
  }

void add_else()
  {
  int psave,c;
  get_command(f,&c);
  if (c!=-2) error("Chybn  syntaxe pý¡kazu else. OŸek v  se '{'");
  was_sub=0;
  if (get_last_level()!=open_level) error("Else bez if");
  add_short(171);
  psave=prog_pos;
  add_short(0);
  pop_and_fill(prog_pos,open_level);
  push_level(open_level,psave);
  open_level++;
  }

void set_alternative(FILE *f)
  {
  int cislo;

  get_cislo(f,&cislo);
  pgf_list[pgf_count-1].alt=cislo+basic_num;
  }

void decode_program(FILE *f)
  {
  int c;
  char next_sub=0;
  char *d;

  d=(char *)calloc(sizeof(char),16384);
  open_level=0;
  c=fgetc(f);
  while (c!=EOF)
     {
     if (c=='!')
        {
        char d[256];

        fgets(&d,256,f);
        decode_pgf1(d);
        }
     else if (isdigit(c) || c=='-')
        {
        int cislo;
        ungetc(c,f);
        get_cislo(f,&cislo);
        add_short(cislo);
        }
     else if (c=='*')
        {
        int cislo;
        get_cislo(f,&cislo);
        if (cislo!=517 && next_sub)
           {
           js_close_sub();
           next_sub=0;
           }
        if (cislo!=171 && !iff_mode) pop_and_fill(prog_pos,open_level);
        switch (cislo)
           {
           case 512:js_dialoge(f);break; //Sekce DIALOG
           case 513:js_first(f);break;   //Prikaz FIRST
           case 514:add_short(152);  //Prikaz WHEN_ITEM
                    copy_short(f);
                    add_short(140);
                    get_cislo(f,&cislo);
                    if (cislo>0)add_short(cislo);else add_short(cislo+32767-basic_num);
                    break;
           case 515:add_short(163); //prikaz WHEN_FLAG
                    copy_short(f);
                    add_short(140);
                    get_cislo(f,&cislo);
                    if (cislo>0)add_short(cislo);else add_short(cislo+32767-basic_num);
                    break;
           case 516:js_standard(f);break; //prikaz STANDARD
           case 517:js_sub(f);next_sub=1;break; //prikaz SUB
           case 520:js_sentence(f);break;   //Sekce SENTENCE
           case -1:js_close_block();break; //znak }//
           case -2: if (iff_mode) add_if_jump();else error("Neocekavany znak '{' (open_block)");
                    open_level++;
                    break; //znak {//
           case 999:save_pgf_name(f);break;
           case 169:if (iff_mode) add_if_jump(); //prikaz if
                    iff_mode=IF_NOT_JUMP;break;
           case 170:if (iff_mode!=IF_NOT_JUMP) error("NOT bez IF"); //modifikator not
                    iff_mode=IF_JUMP;break;
           case 171:if (iff_mode) error("Chybn‚ pou§it¡ pý¡kazu ELSE"); //prikaz else
                    add_else();break;
           case 173:set_alternative(f);break;
           default: add_short(cislo);break;
           }
        }
     else if (c=='$')
        {
        fgets(d,16384,f);
        add_string(d);
        }
     else if (c=='&')
        {
        int cislo;
        get_cislo(f,&cislo);
        add_var(cislo);
        }
     c=fgetc(f);
     }
  free(d);
  }

void save_program()
  {
  FILE *tg;
  int pgstart;

  tg=fopen("dialogy.dat","wb");
  pgstart=pgf_count*sizeof(*pgf_list);
  fwrite(&pgf_count,1,sizeof(pgf_count),tg);
  fwrite(&prog_pos,1,sizeof(prog_pos),tg);
  fwrite(pgf_list,1,pgstart,tg);
  fwrite(program,1,prog_pos,tg);
  fclose(tg);
  }

void create_include_list(char *filename)
  {
  FILE *f,*g;
  int i;
  char s[15];

  f=fopen(filename,"r");
  if (f==NULL)
     {
     printf("Soubor nenalezen: %s\n",filename);
     exit(0);
     }
  g=fopen(TEMP_NAME1,"w");
  fputs("#include dialogy.def\n",g);
  fputs("#command pgf_name *999 1\n",g);
  do
     {
     i=fscanf(f,"%14s",s);
     if (i==1)
        {
        fprintf(g,"pgf_name '");
        while((i=fgetc(f))!='\n' && i!=EOF) fputc(i,g);fputs("'\n",g);
        fprintf(g,"#include %s\n",s);
        libs++;
        i=1;
        }
     }
  while(i==1);
  fclose(f);
  fclose(g);
  }

const char *GetLexLibPath()
{
  static char c[MAX_PATH];
  char *z;
  GetModuleFileName(0,c,MAX_PATH);
  z=strrchr(c,'\\')+1;
  strcpy(z,"lex_lib.exe");
  return c;
}

main(int argc,char *argv[])
  {
  if (argc<2)
     {
     puts("Tento program vyzaduje jmeno souboru, ve kterem se naleza\n"
          "script pro popis dialogu pro hru BRANY SKELDALU v 1.0");
     exit(0);
     }
  puts("");
  puts("Prob¡h  kompilace:");
  puts("   Spouçt¡m program LEX_LIB.EXE\n");
  putenv("DOS4G=QUIET");
  create_include_list(argv[1]);
  if (spawnlp(P_WAIT,GetLexLibPath(),"lex_lib.exe",TEMP_NAME1,"temp.$$$",NULL))
     exit(1);
  if (errno || init("temp.$$$"))
     {
     puts("Nemohu spustit program lex_lib.exe");
     exit(1);
     }
  mlist=fopen(SCRIPT,"w");
  if (mlist==NULL)
     {
     printf("Nemohu otevrit soubor %s pro zapis.\n",SCRIPT);
     exit(0);
     }
  decode_program(f);
  fclose(f);
  remove("temp.$$$");
  remove(TEMP_NAME1);
  puts("Kompilace £spØçn ...");
  printf("Vytvoreno odstavcu: %d\nCelkem knihoven: %d\nCelkem Sentenci: %d\nD‚lka k¢du: %d\n",pgf_count,libs,sentencs,prog_pos);
  save_program();
  fclose(mlist);
  }

