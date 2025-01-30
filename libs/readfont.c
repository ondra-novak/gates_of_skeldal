#include <platform/platform.h>
#include <stdio.h>

#include "types.h"
#include "pcx.h"
#include "memman.h"

int xs,ys;
char *pic;

char font_buffer[65536];
word font_p=0;
word char_table[256];

int top_line;
int bott_line;
int base_line;
int top_ofs;

char znaky[]="0123456789A�BC�D�E��FGHI�JKL��MN�O�PQR�S�T�U��VWXY�Z�"
             "a�bc�d�e��fghi�jkl��mn�o�pqr�s�t�u��vwxy�z�"
             "~!@#$%^&*()_+|-=\\[]{};':,./<>?";

void load_font_pic(char *filename)
  {
  open_pcx(filename,A_8BIT_NOPAL,&pic);
  memcpy(&xs,pic,2);
  memcpy(&ys,pic+2,2);
  top_line=0;
  base_line=0;
  bott_line=-1;
  }

char get_point(int x,int y)
  {
  return *(pic+xs*y+x+6);
  }

int find_lead(int last)
  {
  last++;
  while (last<ys) if (get_point(0,last)!=255) return last;else last++;
  return -1;
  }

int follow_lead(int y)
  {
  int x=0;
  while (get_point(x,y)!=255) x++;
  return x;
  }

int get_top_extend(int lead,int last_top) //vraci nejhorejsi souradnici
  {
  int x;
  last_top++;
  while (last_top<lead)
     {
     for(x=0;x<xs;x++) if (get_point(x,last_top)==0) return last_top;
     last_top++;
     }
  return -1;
  }

int get_bott_extend(int lead)   //vraci nejspodnejsi souradnici
  {
  int x,y,ok;
  y=lead;ok=1;
  while (ok)
     {
     ok=0;
     for(x=0;x<xs;x++) if (get_point(x,y)==0) ok=1;
     y+=ok;
     }
  return y;
  }

char write_accl(char *where,int accl)
  {
  if (accl==0) return 0;
  if (accl==1) *where=0;else *where=accl+6;
  return 1;
  }

void lead_up_line(int *x,int *y)
  {
  base_line=find_lead(base_line);
  if (base_line==-1) return;
  *x=follow_lead(base_line);
  top_line=get_top_extend(base_line,bott_line);
  bott_line=get_bott_extend(base_line);
  *y=base_line;
  }

int find_next_char(int x,int top,int bott)
  {
  int y;

  while (x<xs)
     {
     for (y=top;y<bott;y++)  if (get_point(x,y)==0) return x;
     x++;
     }
  return -1;
  }

char *read_char(char *pos,int *x,int top,int bott)
  {
  char accl=0;
  char *xinfo;
  char space=0;
  int y;

  xinfo=pos++;*xinfo=0;
  *pos++=bott-top+1;
  do
     {
     space=1;
     for (y=top;y<=bott;y++)
        {
        if (get_point(*x,y)==255) accl++;
        else pos+=write_accl(pos,accl),*pos++=2,space=accl=0;
        }
     xinfo[0]++;
     x[0]++;
     }
  while (!space);
  *pos++=255;
  return pos;
  }

word recognize()
  {
  char *zn=znaky;
  int x,y;
  char *fp;

  memset(char_table,0,sizeof(char_table));
  fp=font_buffer;
  lead_up_line(&x,&y);
  while (base_line>=0)
     {
     x=base_line-top_line;
     if (x>top_ofs) top_ofs=x;
     lead_up_line(&x,&y);
     }
  base_line=0;bott_line=-1;
  do
     {
     lead_up_line(&x,&y);
     top_line=base_line-top_ofs;
     if ((x=find_next_char(x,top_line,bott_line))==-1) return fp-font_buffer;
     while (*zn!=0 && x>0)
        {
        char_table[*zn]=(fp-font_buffer)+sizeof(char_table);
        fp=read_char(fp,&x,top_line,bott_line);
        x=find_next_char(x,top_line,bott_line);
        zn++;
        }
     }
  while (*zn);
  return fp-font_buffer;
  }

void save_font(char *filename,word bufsiz)
  {
  FILE *f;

  f=fopen_icase(filename,"wb");
  fwrite(char_table,sizeof(char_table),1,f);
  fwrite(font_buffer,bufsiz,1,f);
  fclose(f);
  }

void main()
  {
  word s;
  load_font_pic("..\\font\\timese2.pcx");
  s=recognize();
  save_font("..\\font\\timese2.fon",s);
  }
