#include <platform/platform.h>
#include <malloc.h>
#include <stdio.h>
#include "pcx.h"
#include "memman.h"

#include <stdarg.h>
#include <string.h>

#define SHADE_STEPS 5
#define SHADE_PAL (SHADE_STEPS*512*2)

void *get_palette_ptr=NULL;


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
        for (j=0;j<i;j++) *trg++=paleta[(uint8_t)*src];
        src++;
        linelen-=i-1;
        }
     else
        *trg++=paleta[(uint8_t)*src++];
     }
  *srcstep=src-srcsave;
  }

void palette_shadow(char *pal1,unsigned short pal2[][256],int tr,int tg,int tb)
  {
  int i,j;
  char *bt;
  int r,g,b;
  short hi;

  for (j=0;j<SHADE_STEPS;j++)
     {
     bt=pal1;
     i=0;
     do
       {
       r=(tr+(*(bt++)-tr)*(3*SHADE_STEPS-3*j-1)/(3*SHADE_STEPS-1))>>3;
       g=(tg+(*(bt++)-tg)*(3*SHADE_STEPS-3*j-1)/(3*SHADE_STEPS-1))>>3;
       b=(tb+(*(bt++)-tb)*(3*SHADE_STEPS-3*j-1)/(3*SHADE_STEPS-1))>>3;
       hi=(r<<11)+(g<<6)+b;
       pal2[j][i]=hi;
       }
    while (++i & 0xff);
    }
  for (j=0;j<SHADE_STEPS;j++)
     {
     bt=pal1;
     i=0;
     do
       {
       r=((*(bt++))*(SHADE_STEPS-j)/SHADE_STEPS)>>3;
       g=((*(bt++))*(SHADE_STEPS-j)/SHADE_STEPS)>>3;
       b=((*(bt++))*(SHADE_STEPS-j)/SHADE_STEPS)>>3;
       hi=(r<<11)+(g<<6)+b;
       pal2[j+SHADE_STEPS][i]=hi;
       }
    while (++i & 0xff);
    }
  }


int load_pcx(char *pcx,int32_t fsize,int conv_type,char **buffer, ... )
  //dale nasleduji int hodnoty poctu prechodu a R,G,B barvy
  {
  unsigned short paleta2[256];
  char *paleta1;
  char *ptr1;
  unsigned short *ptr2;
  char *ptr3;
  int i;
  PCXHEADER pcxdata;
  int xsize,ysize;


  if (pcx==0) return -1;
  paleta1=pcx+fsize-768;
  ptr1=paleta1;ptr2=paleta2;
  if (get_palette_ptr!=NULL)
     memcpy(get_palette_ptr,ptr1,768);
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
  int sz = 0;
  switch (conv_type)
     {
     case A_8BIT: *buffer=(char *)getmem(sz = xsize*ysize+512+16);break;
     case A_16BIT: *buffer=(char *)getmem(sz = xsize*ysize*2+16);break;
     case A_FADE_PAL: *buffer=(char *)getmem(sz = xsize*ysize+SHADE_PAL+16);break;
     case A_8BIT_NOPAL: *buffer=(char *)getmem(sz = xsize*ysize+16);break;
     case A_NORMAL_PAL: *buffer=(char *)getmem(sz = xsize*ysize+16+768);break;
     default: return -2; //invalid type specificied
     }
  ptr1=*buffer;
  *(unsigned short *)ptr1++=xsize;ptr1++;
  *(unsigned short *)ptr1++=ysize;ptr1++;
  *(unsigned short *)ptr1++=conv_type;ptr1++;
  pcx+=sizeof(pcxdata);ptr3=pcx;
  if (conv_type==A_NORMAL_PAL)
     {
     memcpy(ptr1,paleta1,768);
     ptr1+=768;
     }
  if (conv_type==A_8BIT)
     {
     memcpy(ptr1,paleta2,512);
     ptr1+=512;
     }
  if (conv_type==A_FADE_PAL)
     {
     int tr,tg,tb;

     va_list lst;
     va_start(lst, buffer);
     tr=va_arg(lst,int);
     tg=va_arg(lst,int);
     tb=va_arg(lst,int);
     va_end(lst);
     palette_shadow(paleta1,(unsigned short (*)[256])ptr1,tr,tg,tb);
     ptr1+=SHADE_PAL;
     }
  ysize++;
  while (--ysize)
     {
     int step;
     if (conv_type==A_16BIT)
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
  return sz;

}

int open_pcx(const char *filename,int type,char **buffer,...)
  {
  FILE *pcx;
  char *src;
  int32_t fsize;

  pcx=fopen_icase(filename,"rb");
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

