#include <stdio.h>
#include "types.h"

FILE *bmp;
long xsize,ysize,nsize,xcor;
char bmptype;
char *buff,*buff2;
char filename[]="sipka.bmp";
char nfilename[256];
word newpalette[256];
word palshadow[5][256];
char genshadow=0;

int load_file(char *filename)
  {
  long size;

  bmp=fopen(filename,"rb");
  if (!bmp) return -1;
  fseek(bmp,0,SEEK_END);
  size=ftell(bmp);
  fseek(bmp,0,SEEK_SET);
  buff=(void *)malloc(size);
  fread(buff,1,size,bmp);
  fclose(bmp);
  return 0;
  }



void get_bmp_header()
  {
  long *p_long;

  p_long=(long *)(buff+18);
  xsize=*p_long;
  p_long=(long *)(buff+22);
  ysize=*p_long;
  bmptype=*(buff+0x1c);
  if  (bmptype==8)
     xcor=((xsize+3)/4)*4;
  }

int alloc_buffer()
  {
  if (bmptype==24)
      nsize=xsize*ysize*2;
  else
     nsize=xsize*ysize;
  buff2=(void *)malloc(nsize);
  if (buff2==NULL) return -1;
  return 0;
  }

void conv_hicolor()
  {
  char r,g,b;
  short hi;
  char *s,*s1,*t;
  long x,y;

  s=(buff+0x36+xsize*(ysize-1)*3);
  t=buff2;
  for(y=0;y<ysize;y++)
     {
     s1=s;
     for(x=0;x<xsize;x++)
     {
     b=*(s1++)>>3;
     g=*(s1++)>>3;
     r=*(s1++)>>3;
     hi=(r<<10)+(g<<5)+b;
     *(short *)t=hi;
     t+=2;
     }
     s-=xsize*3;
     }
  bmptype=15;
  }

int save_file_hi(char *newname)
  {
  bmp=fopen(newname,"wb");
  if (!bmp) return -1;
  fwrite(&xsize,1,2,bmp);
  fwrite(&ysize,1,2,bmp);
  fwrite(&bmptype,1,2,bmp);
  fwrite(buff2,1,nsize,bmp);
  fclose(bmp);
  return 0;
  }

void pripona(char *input,char *prip,char *output)
  {
  short i,p;
  char *ch;

  i=0;p=0;ch=input;
  while (*ch)
     {
     if (*(ch++)=='.') p=i;
     i++;
     }
 if (!p) p=i;
 ch=input;
 for(i=0;i<p;i++) *(output++)=*(ch++);
 *(output++)='.';
 while (*prip) *(output++)=*(prip++);
 *(output++)='\0';
  }

void conv_palette()
  {
  char i,*bt;
  char r,g,b;
  short hi;

  bt=buff;
  bt+=54;i=0;
  do
     {
     b=*(bt++)>>3;
     g=*(bt++)>>3;
     r=*(bt++)>>3;
     hi=(r<<10)+(g<<5)+b;
     bt++;
     newpalette[i]=hi;
     }
  while (++i);
  }

void palette_shadow(int tr,int tg,int tb)
  {
  char i,j,*bt;
  char r,g,b;
  short hi;

  for (j=0;j<5;j++)
     {
     bt=buff;
     bt+=54;i=0;
     do
       {
       b=(tb+(*(bt++)-tb)*(5-j)/5)>>3;
       g=(tg+(*(bt++)-tg)*(5-j)/5)>>3;
       r=(tr+(*(bt++)-tr)*(5-j)/5)>>3;
       hi=(r<<10)+(g<<5)+b;
       bt++;
       palshadow[j][i]=hi;
       }
    while (++i);
    }
  bmptype=0x28;
  }


void conv_256color()
  {
  char *s,*s1,*t;
  long x,y;

  s=(buff+0x36+1024+xcor*(ysize-1));
  t=buff2;
  for(y=0;y<ysize;y++)
     {
     s1=s;
     for(x=0;x<xsize;x++) *t++=*s1++;
     s-=xcor;
     }
  bmptype=8;
  }

int save_file_256(char *newname)
  {
  bmp=fopen(newname,"wb");
  if (!bmp) return -1;
  fwrite(&xsize,1,2,bmp);
  fwrite(&ysize,1,2,bmp);
  if (genshadow)
     {
     fwrite(&bmptype,1,1,bmp);
     bmptype=0x1;
     fwrite(&bmptype,1,1,bmp);
     fwrite(palshadow,1,sizeof(palshadow),bmp);
     }
  else
     {
     fwrite(&bmptype,1,1,bmp);
     bmptype=0;
     fwrite(&bmptype,1,1,bmp);
     fwrite(newpalette,1,sizeof(newpalette),bmp);
     }
  fwrite(buff2,1,nsize,bmp);
  fclose(bmp);
  return 0;
  }

void help()
  {
  printf("Usage:\n\n BMP2HI filename.bmp [/p] \n");
  printf("If type of BMP is 256 colors use /p for generate palette shadowing.\n");

  return;
  }

void main(int argc, const char *argv[])
  {
  if (argc<2)
     {
     help();
     return;
     }
  if (argc==3 && (argv[2][1]=='p' || argv[2][1]=='P')) genshadow=1;
  if (load_file((char *)argv[1])) return;
  get_bmp_header();
  if (bmptype!=24 && bmptype!=8) return;
  if (alloc_buffer()) return;
  if (bmptype==24)
     {
     conv_hicolor();
     pripona((char *)argv[1],"HI",nfilename);
     save_file_hi(nfilename);
     }
  else
     {
      conv_256color();
      if (genshadow) palette_shadow(0,0,0); else conv_palette();
      pripona((char *)argv[1],"HI",nfilename);
     save_file_256(nfilename);
     }
  printf("Konverze z %s na %s uspesna",argv[1],nfilename);
  return ;
  }
