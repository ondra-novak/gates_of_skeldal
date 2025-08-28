#include <platform/platform.h>
#include <libs/cztable.h>
/*

 Popis jazyka pro psani textu do knihy


 [cislo]

 .... text
 ....

 [/cislo]


 Tagy

 <P> paragraph
 <BR> break line
 <IMG SRC="filename" ALIGN=/left,right,center/>
 <HR> horizontal rule
 <EP> page end



 -----------------------------

 Vnitrni zapis

 Escape sekvence

 ESC s <vynechat>  - mezera n bodu
 ESC p <filename> <x> <y> obrazek na souradnicich x a y
 ESC e - konec stranky (jako prvni v textu)
 ESC h - horizontalni rule (jako prvni v textu)
 ESC l <skip> - konec radky (skip je pocet vynechanych bodu)

 Zapis cisla - to je hexa cislo od 1-255 pokud se nevejde do rozsahu je po 256
 pridana dalsi hodnota, tj 255 se zapise jako 255,1,
                           350 se zapise jako 255,96,

*/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <libs/strlite.h>
#include <libs/bgraph.h>
#include <libs/memman.h>
#include <libs/event.h>
#include "globals.h"

#define XMAX 254
#define YMAX 390
#define XLEFT 34
#define YLEFT 50
#define XRIGHT 354

#define PARAGRAPH "P"
#define BREAKLINE "BR"
#define IMAGE "IMG"
#define HOR_RULE "HR"
#define CENTER1 "CENTER"
#define CENTER2 "/CENTER"
#define DISTEND1 "DISTEND"
#define DISTEND2 "/DISTEND"
#define ALIGN "ALIGN"
#define PIC_LINE "LINE"
#define PIC_LSIZ "LSIZE"
#define SRC "SRC"

#define ALEFT "LEFT"
#define ARIGHT "RIGHT"
#define ACENTER "CENTER"

#define ANUM_LEFT 1
#define ANUM_RIGHT 2
#define ANUM_CENTER 0

#define END_PAGE 'e'
#define SPACE 's'
#define PICTURE 'p'
#define HRUL  'h'
#define END_LINE 'l'

#define BOOK_FILE "_BOOK.TMP"

static int center=0;
static int distend=0;
static TSTR_LIST all_text=NULL;
static char read_buff[256];
static char write_buff[256];
static int buff_pos=0;
static int buff_end=0;
static int total_width=XMAX;
static int left_skip=0;
static int linepos=0,last_skip=1;
static int picture_len=0;
static char winconv=0;
static int relpos=0;

static int insert_num(char *text,int pos,int num)
  {
  char c=0x80;
  do
     {
     c=num & 0x3f;num>>=6;
     if (num) c|=0x80;
     c|=0x40;
     text[pos++]=c;
     }
  while (num);
  return pos;
  }


static int read_num(char *text,int *pos)
  {
  int num=0,shift=0;
  char c;

  do
     {
     c=text[pos[0]++];
     num|=(c & 0x3f)<<shift;
     shift+=6;
     }
  while (c & 0x80);
  return num;
  }

static void next_line(int step)
  {
  linepos+=step;
   if (linepos>YMAX)
     {
     char s[3];
     s[0]=27;
     s[1]=END_PAGE;
     s[2]=0;
     str_add(&all_text,s);
     linepos=0;
     picture_len=-1;
     }
  last_skip=step;
  }


static int insert_end_line_and_save(int p,int ys)
  {
  int size;
  while (read_buff[buff_pos]==' ' && buff_pos<buff_end) buff_pos++;
  size=buff_end-buff_pos;
  if (size)memcpy(read_buff,read_buff+buff_pos,size);
  write_buff[p++]=27;
  write_buff[p++]=END_LINE;
  p=insert_num(write_buff,p,ys);
  write_buff[p++]=0;
  str_add(&all_text,write_buff);
  buff_pos=size;
  buff_end=buff_pos;
  if (picture_len>0) picture_len-=ys;
  if (picture_len<=0 && total_width!=XMAX)
     {
     picture_len=0;
     total_width=XMAX;
     left_skip=0;
     }
  return p;
  }

static int insert_left_skip(int p,int skip)
  {
  skip+=left_skip;
  if (skip)
     {
     write_buff[p++]=27;
     write_buff[p++]=SPACE;
     p=insert_num(write_buff,p,skip);
     }
  return p;
  }

static void save_line_oboustrane()
  {
  char space[]=" ";
  int xs,ys,ss,mez,mm=0,mc=0;
  int i,p;

  for(i=0,xs=0,mez=0,ys=0;i<buff_pos;i++)
     {
     space[0]=read_buff[i];
     if (space[0]==32) mez++;else
        {
        xs+=text_width(space);
        p=text_height(space);
        if (p>ys) ys=p;
        }
     }
  ss=total_width-xs;
  p=0;
  if (!ys) ys=last_skip;
  next_line(ys);
  p=insert_left_skip(p,0);
  for(i=0,mc=0;i<buff_pos;i++)
     {
     if (read_buff[i]==32)
        {
        int sp;
        write_buff[p++]=27;
        write_buff[p++]=SPACE;
        mc++;
        sp=ss*(mc)/mez;
        p=insert_num(write_buff,p,sp-mm);
        mm=sp;
        }
     else write_buff[p++]=read_buff[i];
     }
  insert_end_line_and_save(p,ys);
  }

static void save_line_center()
  {
  char space[]=" ";
  int xs,ys,ss;
  int i,p;

  for(i=0,xs=0,ys=0;i<buff_pos;i++)
     {
     space[0]=read_buff[i];
        xs+=text_width(space);
        p=text_height(space);
        if (p>ys) ys=p;
     }
  p=0;
  if (!ys) ys=last_skip;
  next_line(ys);
  ss=total_width-xs;
  p=insert_left_skip(p,ss/2);
  memcpy(write_buff+p,read_buff,buff_pos);
  p+=buff_pos;
  write_buff[p]=0;
  insert_end_line_and_save(p,ys);
  }

static void save_line_left()
  {
  int p,z;
  int ys;

  p=0;
  p=insert_left_skip(p,0);z=p;
  if (buff_pos) memcpy(write_buff+p,read_buff,buff_pos);
  p+=buff_pos;
  write_buff[p]=0;
  ys=text_height(write_buff+z);
  if (!ys) ys=last_skip;
  next_line(ys);
  insert_end_line_and_save(p,ys);
  }


static void save_buffer()
  {
  while (buff_end>buff_pos && read_buff[buff_end]==32) buff_end--;
  if (center) save_line_center();
     else if (buff_pos==buff_end || !distend) save_line_left(); else save_line_oboustrane();
  }

static void break_line()
  {
  buff_pos=buff_end;
  save_buffer();
  }

static char read_set(TMPFILE_RD *txt,char *var,char *set)
  {
  int c;
  char *cc;
  char d;
  char e;

  temp_storage_scanf(txt,"%[^=]%c",var,&d);
  do
     c=temp_storage_getc(txt);
  while (c<33);
  if (c=='"') temp_storage_scanf(txt,"%[^\"]%c%c",set,&d,&e);
  else if (c=='\'') temp_storage_scanf(txt,"%[^']%c%c",set,&d,&e);
  else
     {
      temp_storage_ungetc(txt);
      temp_storage_scanf(txt,"%[^> ]%c",set,&e);
     }
  c = e;
  while(c<33 && c!=EOF) c=temp_storage_getc(txt);
  if (c!='>') temp_storage_ungetc(txt);
  cc=strchr(var,0);
  while (cc!=var)
     {
     cc--;
     if (*cc>32)
        {
        cc++;
        break;
        }
     }
  *cc=0;
  strupper(set);
  strupper(var);
  return c;
  }

static int get_data_handle(char *filename,ABLOCK_DECODEPROC  dec)
  {
  int i;

  i=find_handle(filename,dec);
  if (i==-1)
     {
     i=end_ptr++;
     def_handle(i,filename,dec,SR_DIALOGS);
     }
  return i;
  }

static void insert_picture(char *filename,int align,int line,int lsize)
  {
  int x=0, y=0;
  const short *psiz;
  char *c=write_buff;

  psiz=ablock(get_data_handle(filename,pcx_8bit_decomp));
  switch (align)
     {
     case ANUM_CENTER:
                       x=(XMAX-psiz[0])/2;
                       y=linepos;
                       linepos+=psiz[1];
                       *c++=27;
                       *c++=END_LINE;
                       c+=insert_num(c,0,psiz[1]+last_skip);
                       break;
     case ANUM_LEFT:   x=0;
                       y=linepos;
                       left_skip=psiz[0]+5;
                       total_width=XMAX-left_skip;
                       break;
     case ANUM_RIGHT:  total_width=(x=XMAX-psiz[0])-5;
                       left_skip=0;
                       y=linepos;
                       break;
     }
  if (!lsize) lsize=psiz[1]-line;
  picture_len=lsize;
  *c++=27;
  *c++=PICTURE;
  while (*filename) *c++=*filename++;
  *c++=':';
  c+=insert_num(c,0,x);
  c+=insert_num(c,0,y);
  c+=insert_num(c,0,line);
  c+=insert_num(c,0,lsize);
  *c++=0;
  str_add(&all_text,write_buff);
  }

static char read_tag(TMPFILE_RD *txt)
  {
  char c,var[256],set[256];
  int i;

  i=temp_storage_scanf(txt,"%[^> ] %c",var,&c);
  while(c<33 && i!=EOF) c=i=temp_storage_getc(txt);
  if (c!='>') temp_storage_ungetc(txt);
  strupper(var);
  if (!strcmp(var,PARAGRAPH))
     {
     break_line();
     break_line();
     return 1;
     }
  if (!strcmp(var,BREAKLINE))
     {
     break_line();
     return 1;
     }
  if (!strcmp(var,IMAGE))
     {
     char pic_name[50]=" ";
     char alig=0;
     int line=0,lsize=0;

     while (c!='>')
        {
        c=read_set(txt,var,set);
        if (!strcmp(var,SRC)) strcopy_n(pic_name,set,49);
        else if (!strcmp(var,ALIGN))
           {
           if (!strcmp(set,ALEFT)) alig=1;
           else if (!strcmp(set,ARIGHT)) alig=2;
           else if (!strcmp(set,ACENTER)) alig=0;
           }
        else if (!strcmp(var,PIC_LINE)) sscanf(set,"%d",&line);
        else if (!strcmp(var,PIC_LSIZ)) sscanf(set,"%d",&lsize);
        }
     if (pic_name[0]!=0)
        insert_picture(pic_name,alig,line,lsize);
     return 0;
     }
  if (!strcmp(var,CENTER1)) center++;
  else if (!strcmp(var,CENTER2))
     {
     if (center>0) center--;
     }
  else if (!strcmp(var,DISTEND1)) distend++;
  else if (!strcmp(var,DISTEND2))
     {
     if (distend>0) distend--;
     }
  return 0;
  }


static char skip_section(TMPFILE_RD *txt)
  {
  int c;
  char end=1;

  c=temp_storage_getc(txt);
  while (c!=']' && c!=EOF)
     {
     c=temp_storage_getc(txt);
     end=0;
     }
  if (c==EOF) end=1;
  return end;
  }

void prekodovat(char *c)
  {
    windows2kamenik(c, strlen(c), c);
  }

static void read_text(TMPFILE_RD *txt)
  {
  int i = 0;
  int xs;
  char ss[2]=" ";
  char wsp=1;

  buff_pos=0;
  buff_end=0;
  xs=0;
  do
     {
     i=temp_storage_getc(txt);
     if (i==EOF) break;
     if (i<32) i=32;
     if (i=='<')
        {
        if (read_tag(txt))
           {
           xs=0;
           wsp=1;
           }
        continue;
        }
     if (i=='[')
        {
        if (skip_section(txt)) break;
        continue;
        }
     if (i==32)
        {
        if (wsp) continue;
        buff_pos=buff_end;
        wsp=1;
        }
     else wsp=0;
     if (i=='&') i=temp_storage_getc(txt);
     if (winconv) i=windows2kamenik_chr(i);
     ss[0]=i;
     xs+=text_width(ss);
     read_buff[buff_end++]=i;
     if (xs>total_width && !wsp)
        {
        save_buffer();
        read_buff[buff_end]=0;
        xs=text_width(read_buff);
        }
     }
  while (1);
  }

static void seek_section(TMPFILE_RD *txt,int sect_number)
  {
  int c=0,i=0;

  winconv=0;
  do
     {
     while (c!='[' && c!=EOF) c=temp_storage_getc(txt);
     if (c == EOF) break;
     if (c=='[')
       {
       i=-2;
       c = temp_storage_getc(txt);
       if (c>='0' && c<='9') {
           i = 0;
           while (c>='0' && c<='9') {
               i = i * 10 +(c - '0');
               c = temp_storage_getc(txt);
           }
       }
       if (i==sect_number)
          {
          while(c!=']')
             {
             if (c=='W' || c=='w') winconv=1;
             if (c=='K' || c=='k') winconv=0;
             c=temp_storage_getc(txt);
             }
          return;
          }
       }
     c=0;
     }
  while (i!=EOF);
	{
    char buff[256];
	sprintf(buff,"Nemohu najit odstavec s cislem %d.",sect_number);
	display_error(buff);
	}
  exit(1);
  }

void add_text_to_book(const char *filename,int odst)
  {
  TMPFILE_RD *fl;

  set_font(H_FKNIHA,NOSHADOW(0));
  if (all_text==NULL) all_text=create_list(256);
  fl=enc_open(filename);
  if (fl==NULL) return;
  seek_section(fl,odst);
  read_text(fl);
  next_line(1000);
  enc_close(fl);
  }

static char *displ_picture(char *c)
  {
  char *d;
  int x,y,hn,z,ln,sl;
  const short *sh;
  int32_t scr_linelen2 = GetScreenPitch();


  d=write_buff;
  while (*c!=':') *d++=*c++;
  *d++=0;c++;
  hn=get_data_handle(write_buff,pcx_8bit_decomp);
  x=read_num(c,(z=0,&z));c+=z;
  y=read_num(c,(z=0,&z));c+=z;
  ln=read_num(c,(z=0,&z));c+=z;
  sl=read_num(c,(z=0,&z));c+=z;
  sh=ablock(hn);
  if (sh[1]+y>YMAX) return c;
  y+=YLEFT;
  x+=relpos;
  put_8bit_clipped(sh,GetScreenAdr()+x+scr_linelen2*y,ln,sh[0],sl);
  return c;
  }

void write_book(int page)
  {
  int i=0,y=0,z,zz,ps,pg;
  char *c;
  char space[]=" ";

  pg=page;
  if (all_text==NULL) return;
  set_font(H_FKNIHA,NOSHADOW(0));
  relpos=XLEFT;
  zz=str_count(all_text);
  if (--page)
  for(i=0;i<zz && page;i++)
     {
     c=all_text[i];
     if (c!=NULL && c[0]==27 && c[1]==END_PAGE) page--;
     }
  if (page) return;
  for(ps=0;ps<2;ps++)
     {
  position(relpos,YLEFT+y);
  do
     {
     if (i>zz) break;
     c=all_text[i];
     if (c==NULL) break;
     if (c[0]==27 && c[1]==END_PAGE) break;
     while (*c)
        {
        z=0;
        if (*c==27)
           {
           c++;
           switch (*c++)
              {
              case SPACE:
                          rel_position_x(read_num(c,&z));
                          c+=z;
                          break;
              case END_LINE:
                          y+=read_num(c,&z);
                          position(relpos,YLEFT+y);
                          c+=z;
                          break;
              case PICTURE:
                          c=displ_picture(c);
                          break;
              }
           }
        else
           {
           space[0]=*c++;
           outtext(space);
           }
        }
     i++;
     }
  while (1);
  i++;y=0;
  relpos=XRIGHT;
  if (ps==0)
     {
     char s[20];
     sprintf(s,texty[135],pg);
     set_aligned_position(XLEFT,YLEFT+YMAX,0,0,s);
     outtext(s);
     }
  if (ps==1)
     {
     char s[20];
     sprintf(s,texty[136],pg+1);
     set_aligned_position(XRIGHT+XMAX,YLEFT+YMAX,2,0,s);
     outtext(s);
     }
     }
  }

int count_pages()
  {
  int i,cn,z;
  char *c;

  if (all_text==NULL) return 0;
  z=str_count(all_text);
  for(i=0,cn=0;i<z;i++)
     {
     c=all_text[i];
     if (c!=NULL && c[0]==27 && c[1]==END_PAGE) cn++;
     }
  return cn;
  }

void save_book()
  {
  TMPFILE_WR *f;
  int i,ss;
  char *tx;

  if (all_text==NULL) return;

  f = temp_storage_create(BOOK_FILE);
  i=0;
  ss=str_count(all_text);
  while (i<ss && (tx=all_text[i++])!=NULL)
     {
      temp_storage_write(tx,strlen(tx), f);
      temp_storage_write("\n",1, f);
     }
  temp_storage_close_wr(f);
  }

void load_book()
  {

  if (all_text!=NULL) release_list(all_text);
  all_text=NULL;
  int sz = temp_storage_find(BOOK_FILE);
  if (sz < 0) return;
  char *data = getmem(sz);
  temp_storage_retrieve(BOOK_FILE, data, sz);
  int b = 0;
  int e;
  for (e = 0; e < sz; ++e) {
      if (data[e] == '\n') {
          data[e] = 0;
          str_add(&all_text, data+b);
          b = e+1;
      }
  }
  if (b < e) {
      str_add(&all_text, data+b);
  }
  free(data);
  }


