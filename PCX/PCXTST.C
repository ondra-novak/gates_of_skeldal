#include "..\types.h"
#include <malloc.h>
#include <mem.h>
#include <stdio.h>
#include "..\bgraph.h"
#define A_8BIT 8
#define A_15BIT 15
#define A_FADE_PAL 256*8

typedef struct pcxrecord
     {
     unsigned short id;
     char encoding;
     char bitperpixel;
     unsigned short xmin,ymin,xmax,ymax;
     unsigned short hdpi,vdpi;
     char colormap[48];
     char reserved;
     char mplanes;
     unsigned short bytesperline;
     unsigned short paleteinfo;
     unsigned short hscreen,vscreen;
     char filler[54];
     }PCXHEADER;



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


int load_pcx(FILE *pcx,long fsize,int conv_type,char **buffer, ... )
  //dale nasleduji int hodnoty poctu prechodu a R,G,B barvy
  {
  long curptr;
  char *src_buff;
  char paleta1[256][3];
  unsigned short paleta2[256];
  char *ptr1;unsigned short *ptr2;
  char *ptr3;
  int i;
  PCXHEADER pcxdata;
  int xsize,ysize;


  curptr=ftell(pcx);
  fseek(pcx,fsize-768,SEEK_SET);
  fread(&paleta1,768,1,pcx);
  ptr1=paleta1;ptr2=paleta2;
  for (i=0;i<256;i++)
     {
     *ptr2=*(ptr1++)>>3;
     *ptr2=(*ptr2<<5)+(*(ptr1++)>>3);
     *ptr2=(*ptr2<<5)+(*(ptr1++)>>3);
     ptr2++;
     }

  fseek(pcx,curptr,SEEK_SET);
  fread(&pcxdata,sizeof(pcxdata),1,pcx);
  xsize=pcxdata.xmax-pcxdata.xmin+1;
  ysize=pcxdata.ymax-pcxdata.ymin+1;
  switch (conv_type)
     {
     case A_8BIT: *buffer=(char *)malloc(xsize*ysize+512+6+16);break;
     case A_15BIT: *buffer=(char *)malloc(xsize*ysize*2+6+16);break;
     case A_FADE_PAL: *buffer=(char *)malloc(xsize*ysize+512*5+6+16);break;
     default: return -2; //invalid type specificied
     }
  ptr1=*buffer;
  *(word *)ptr1++=xsize;ptr1++;
  *(word *)ptr1++=ysize;ptr1++;
  *(word *)ptr1++=conv_type;ptr1++;
  src_buff=(char *)malloc(fsize);ptr3=src_buff;
  fread(src_buff,fsize-768-sizeof(pcxdata),1,pcx);
  if (conv_type==A_8BIT)
     {
     memcpy(ptr1,paleta2,512);
     ptr1+=512;
     }
  while (--ysize)
     {
     int step;
     if (conv_type==15)
        {
        decomprimate_line_hi(ptr3,(word *)ptr1,paleta2,pcxdata.bytesperline,&step);
        ptr1+=2*xsize;
        }
     else
        {
        decomprimate_line_256(ptr3,ptr1,pcxdata.bytesperline,&step);
        ptr1+=xsize;
        }
     ptr3+=step;
     }
  free(src_buff);
  return 0;
  }

int open_pcx(char *filename,int type,char **buffer)
  {
  FILE *pcx;
  long fsize;

  pcx=fopen(filename,"rb");
  if (pcx==NULL) return -1;
  fseek(pcx,0,SEEK_END);
  fsize=ftell(pcx);
  fseek(pcx,0,SEEK_SET);
  fsize=load_pcx(pcx,fsize,type,buffer);
  fclose(pcx);
  return fsize;
  }

void initmode32b();

main()
  {
  char *buf;

  initmode32b();
  getchar();
  //open_pcx("d:\\tycoon2\\scr2.pcx",A_8BIT,&buf);
  //put_picture(0,0,buf);free(buf);
  open_pcx("desk.pcx",A_15BIT,&buf);
  put_picture(0,480-101,buf);free(buf);
  open_pcx("topbar.pcx",A_15BIT,&buf);
  put_picture(0,0,buf);free(buf);

  showview(0,0,0,0);
  getchar();
  return 0;
  }





