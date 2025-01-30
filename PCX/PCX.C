#include <skeldal_win.h>
#include <malloc.h>
#include <mem.h>
#include <stdio.h>
/*#include "..\types.h"*/
#include "pcx.h"
#include <libs/memman.h>
/*#include "..\bgraph.h"*/


void decomprimate_line_256(char *src,char *trg,int linelen,int *srcstep)
  {
  char *srcsave;

  srcsave=src;
  while (linelen--)
     {
     if (*src>=0xc0)
        {
        int i;
        i=*src++ & 0x3f;memset(trg,*src++,i);
        trg+=i;linelen-=i-1;
        }
     else
        *trg++=*src++;
     }
  *srcstep=src-srcsave;
  }
void decomprimate_line_hi(char *src,unsigned short *trg,unsigned short *paleta,int linelen,int *srcstep)
  {
  char *srcsave;

  srcsave=src;
  while (linelen--)
     {
     if (*src>=0xc0)
        {
        int i,j;
        i=(*src++) & 0x3f;
        for (j=0;j<i;j++) *trg++=paleta[*src];
        src++;
        linelen-=i-1;
        }
     else
        *trg++=paleta[*src++];
     }
  *srcstep=src-srcsave;
  }

void palette_shadow(char *pal1,unsigned short pal2[][256],int tr,int tg,int tb)
  {
  int i,j;
  char *bt;
  int r,g,b;
  short hi;

  for (j=0;j<5;j++)
     {
     bt=pal1;
     i=0;
     do
       {
       r=(tr+(*(bt++)-tr)*(5-j)/5)>>3;
       g=(tg+(*(bt++)-tg)*(5-j)/5)>>3;
       b=(tb+(*(bt++)-tb)*(5-j)/5)>>3;
       hi=(r<<10)+(g<<5)+b;
       pal2[j][i]=hi;
       }
    while (++i & 0xff);
    }
  }


int load_pcx(char *pcx,long fsize,int conv_type,char **buffer, ... )
  //dale nasleduji int hodnoty poctu prechodu a R,G,B barvy
  {
  char *paleta1;
  unsigned short paleta2[256];
  char *ptr1;unsigned short *ptr2;
  char *ptr3;
  int i;
  PCXHEADER pcxdata;
  int xsize,ysize;


  paleta1=pcx+fsize-768;
  ptr1=paleta1;ptr2=paleta2;
  for (i=0;i<256;i++)
     {
     *ptr2=*(ptr1++)>>3;
     *ptr2=(*ptr2<<5)+(*(ptr1++)>>3);
     *ptr2=(*ptr2<<6)+(*(ptr1++)>>3);
     ptr2++;
     }

  memcpy(&pcxdata,pcx,sizeof(pcxdata));
  xsize=pcxdata.xmax-pcxdata.xmin+1;
  ysize=pcxdata.ymax-pcxdata.ymin+1;
  switch (conv_type)
     {
     case A_8BIT: *buffer=(char *)getmem(xsize*ysize+512+6+16);break;
     case A_15BIT: *buffer=(char *)getmem(xsize*ysize*2+6+16);break;
     case A_FADE_PAL: *buffer=(char *)getmem(xsize*ysize+512*5+6+16);break;
     default: return -2; //invalid type specificied
     }
  ptr1=*buffer;
  *(unsigned short *)ptr1++=xsize;ptr1++;
  *(unsigned short *)ptr1++=ysize;ptr1++;
  *(unsigned short *)ptr1++=conv_type;ptr1++;
  pcx+=sizeof(pcxdata);ptr3=pcx;
  if (conv_type==A_8BIT)
     {
     memcpy(ptr1,paleta2,512);
     ptr1+=512;
     }
  if (conv_type==A_FADE_PAL)
     {
     int *i,tr,tg,tb;

     i=(int *)&buffer;i++;
     tr=*i++;tg=*i++;tb=*i++;

     palette_shadow(paleta1,(unsigned short *)ptr1,tr,tg,tb);
     ptr1+=5*512;
     }
  ysize++;
  while (--ysize)
     {
     int step;
     if (conv_type==15)
        {
        decomprimate_line_hi(ptr3,(unsigned short *)ptr1,paleta2,pcxdata.bytesperline,&step);
        ptr1+=2*xsize;
        }
     else
        {
        decomprimate_line_256(ptr3,ptr1,pcxdata.bytesperline,&step);
        ptr1+=xsize;
        }
     ptr3+=step;
     }
  return 0;
  }

int open_pcx(char *filename,int type,char **buffer,...)
  {
  FILE *pcx;
  char *src;
  long fsize;

  pcx=fopen(filename,"rb");
  if (pcx==NULL) return -1;
  fseek(pcx,0,SEEK_END);
  fsize=ftell(pcx);
  fseek(pcx,0,SEEK_SET);
  src=(char *)getmem(fsize);
  fread(src,1,fsize,pcx);
  fsize=load_pcx(src,fsize,type,buffer,*((int *)&buffer+1),*((int *)&buffer+2),*((int *)&buffer+3));
  fclose(pcx);
  free(src);
  return fsize;
  }

/*void initmode32b();

main()
  {
  char *buf;

  initmode32b();
  open_pcx("DESK.pcx",A_8BIT,&buf,0,0,0);
  put_picture(0,480-102,buf);
  showview(0,0,0,0);
  getchar();
  return 0;
  }


*/

