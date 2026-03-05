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


char is_pcx(const unsigned char *h, size_t size)
{
    if (size < 128) return 0;

    if (h[0] != 0x0A) return 0;
    if (h[2] != 0x01) return 0;

    if (!(h[3] == 1 || h[3] == 2 || h[3] == 4 || h[3] == 8))
        return 0;

    uint16_t xmin = h[4]  | (h[5]  << 8);
    uint16_t ymin = h[6]  | (h[7]  << 8);
    uint16_t xmax = h[8]  | (h[9]  << 8);
    uint16_t ymax = h[10] | (h[11] << 8);

    if (xmax < xmin || ymax < ymin)
        return 0;

    return 1;
}

void decomprimate_line_256(const char *src,char *trg,int linelen,int *srcstep)
  {
  const char *srcsave;

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
void decomprimate_line_hi(const char *src,unsigned short *trg,unsigned short *paleta,int linelen,int *srcstep)
  {
  const char *srcsave;

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

static inline int color_clamp(float f) {
    if (f > 255.0) f = 255;
    return (int)f;
}

void palette_shadow(const char *pal1,unsigned short pal2[][256],int tr,int tg,int tb, float bright, float fend)
  {
    float fmult = MIN(1.0f, bright);
    float cmult = MAX(1.0f, bright);


      for (int k = 0; k < 2; k++) {
          for (int  j=0;j<SHADE_STEPS;j++) {
             const unsigned char *bt=(const unsigned char *)pal1;
             float f = fmult*(3*SHADE_STEPS-3*j-1)/(3*SHADE_STEPS-1)*fend+1-fend;
             for (int i = 0; i < 256; ++ i) {
               int r=color_clamp((tr+(*(bt++)*cmult-tr)*f))>>3;
               int g=color_clamp((tg+(*(bt++)*cmult-tg)*f))>>3;
               int b=color_clamp((tb+(*(bt++)*cmult-tb)*f))>>3;
               short hi=RGB555(r,g,b);
               pal2[j+SHADE_STEPS*k][i]=hi;
             }
          }
          tr = 0;
          tg = 0;
          tb = 0;
      }
  }

int load_pcx(const char *pcx,int32_t fsize,int conv_type,char **buffer, ... )
  //dale nasleduji int hodnoty poctu prechodu a R,G,B barvy
  {
  unsigned short paleta2[256];
  const char *paleta1;
  const char *ptr1;
  char *ptr4;
  unsigned short *ptr2;
  const char *ptr3;
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
      int r = ptr1[0];
      int g = ptr1[1];
      int b = ptr1[2];
      *ptr2 = RGB888(r,g,b);
      ++ptr2;
      ptr1+=3;
     }

  memcpy(&pcxdata,pcx,sizeof(pcxdata));
  xsize=pcxdata.xmax-pcxdata.xmin+1;
  ysize=pcxdata.ymax-pcxdata.ymin+1;
  int sz = 0;
  switch (conv_type)
     {
     case A_8BIT: *buffer=(char *)getmem(sz = xsize*ysize+512+16);break;
     case A_16BIT_ZERO_TRANSP:conv_type = A_16BIT; paleta2[0] = 0x8000;CASE_FALLTHROUGH;
     case A_16BIT: *buffer=(char *)getmem(sz = xsize*ysize*2+16);break;
     case A_FADE_PAL: *buffer=(char *)getmem(sz = xsize*ysize+SHADE_PAL+16);break;
     case A_8BIT_NOPAL: *buffer=(char *)getmem(sz = xsize*ysize+16);break;
     case A_NORMAL_PAL: *buffer=(char *)getmem(sz = xsize*ysize+16+768);break;
     default: return -2; //invalid type specificied
     }
  ptr4=*buffer;
  *(unsigned short *)ptr4++=xsize;ptr4++;
  *(unsigned short *)ptr4++=ysize;ptr4++;
  *(unsigned short *)ptr4++=conv_type;ptr4++;
  pcx+=sizeof(pcxdata);ptr3=pcx;
  if (conv_type==A_NORMAL_PAL)
     {
     memcpy(ptr4,paleta1,768);
     ptr4+=768;
     }
  if (conv_type==A_8BIT)
     {
     memcpy(ptr4,paleta2,512);
     ptr4+=512;
     }
  if (conv_type==A_FADE_PAL)
     {
     int tr,tg,tb;
     float factor_mlt;
     float factor_end;

     va_list lst;
     va_start(lst, buffer);
     tr=va_arg(lst,int);
     tg=va_arg(lst,int);
     tb=va_arg(lst,int);
     factor_mlt=(float)va_arg(lst,double);
     factor_end=(float)va_arg(lst,double);
     va_end(lst);
     palette_shadow(paleta1,(unsigned short (*)[256])ptr4,tr,tg,tb,factor_mlt, factor_end);
     ptr4+=SHADE_PAL;
     }
  ysize++;
  while (--ysize)
     {
     int step;
     if (conv_type==A_16BIT)
        {
        decomprimate_line_hi(ptr3,(unsigned short *)ptr4,paleta2,pcxdata.bytesperline,&step);
        ptr4+=2*xsize;
        }
     else
        {
        decomprimate_line_256(ptr3,ptr4,pcxdata.bytesperline,&step);
        ptr4+=xsize;
        }
     ptr3+=step;
     }
  return sz;

}
/*
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
  va_list lst;
  va_start(lst, buffer);
  int
  fsize=load_pcx(src,fsize,type,buffer,*((int *)&buffer+1),*((int *)&buffer+2),*((int *)&buffer+3));
  fclose(pcx);
  free(src);
  return fsize;
  }
*/
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

