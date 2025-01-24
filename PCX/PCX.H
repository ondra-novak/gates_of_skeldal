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



int load_pcx(char *pcx,long fsize,int conv_type,char **buffer, ... );
int open_pcx(char *filename,int type,char **buffer,...);

