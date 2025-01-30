#ifndef _PCX_H_
#define _PCX_H_

#ifdef __cplusplus
extern "C" {
#endif

#define A_8BIT 8
#define A_16BIT 16
#define A_FADE_PAL (256+8)
#define A_8BIT_NOPAL (512+8)
#define A_NORMAL_PAL (768+8)

#pragma pack(1)
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
#pragma pack()


//returns <0 error, >0 allocated size
int load_pcx(char *pcx,int32_t fsize,int conv_type,char **buffer, ... );
int open_pcx(const char *filename,int type,char **buffer,...);
void palette_shadow(char *pal1,unsigned short pal2[][256],int tr,int tg,int tb);
extern void *get_palette_ptr;

#ifdef __cplusplus
}
#endif
#endif
