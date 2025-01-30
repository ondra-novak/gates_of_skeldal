#include <libs/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libs/bgraph.h>
#include <i86.h>

#include "anipack.h"
#include "cinema.h"
#include <libs/memman.h>


#define CINEMA ".cnm"
#define PLATNO_X 640
#define PLATNO_Y 184

#define PLATNO_S (PLATNO_X*PLATNO_Y)
#define POZICE (60)

#define DECOMP_BUFF 150000

FILE *anim;
word *playscreen;
word *playscreen2;
word *paleta;
word *show_place;
char *decomp_buff;
char *ip;
word direct_line_len=640*8;
char direct=0;
char fusing=1;
char ifuse=0;
char interlaced=0;
char sdiff=0;

char test_frame[120000];

void priponu(char *source,char *target,int n)
  {
  char *c,*d;

  c=strrchr(source,'.');
  d=strrchr(source,'\\');
  strncpy(target,source,n);
  if (c==NULL || c<d) strcat(target,CINEMA);
  }


char col_table[]={0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};

void decompr_box(word *adr)
  {
  char mode;
  word palxlat[16];
  int i;

  mode=*ip++;
  if (mode==0)
     {
     if ((*ip)--)
        {
        ip--;
        return;
        }
     *ip=0;
     mode=1;
     }
  if (mode==255)
     {
     ip--;
     return;
     }
  if ((mode>16 && mode<64) || mode>64) exit(0);
  if (mode<64)
     {
     for(i=0;i<mode;i++) palxlat[i]=paleta[*ip++];
     mode=col_table[mode-1];
     }
  else mode=8;
  switch (mode)
     {
     case 0:load_0bit(adr,palxlat);break;
     case 1:ip=load_1bit(ip,adr,palxlat);break;
     case 2:ip=load_2bit(ip,adr,palxlat);break;
     case 3:ip=load_3bit(ip,adr,palxlat);break;
     case 4:ip=load_4bit(ip,adr,palxlat);break;
     case 8:ip=load_8bit(ip,adr,paleta);break;
     }
  }

void decompr_pict()
  {
  int x;
  int y,yy;

  ip=decomp_buff;
  yy=0;
  for (y=0;y<23;y++,yy+=direct_line_len)
     for (x=0;x<640;x+=8)
       decompr_box(playscreen+x+yy);
  }



void load_frame(FILE *anim)
  {
  int size;
  fread(&size,1,sizeof(size),anim);
  fread(decomp_buff,1,size,anim);
  decomp_buff[size]=0xff;
  memcpy(test_frame,decomp_buff,size);
  }


void load_palette(FILE *anim)
  {
  word i;
  word size;
  fread(&i,1,2,anim);
  fread(&size,1,2,anim);
  fread(&paleta[i],2,size,anim);
  //if (fusing) for(i=1;i<256;i++) paleta[i]&=0x7bdf;
  if (sdiff) paleta[0]=0;
  }


void init_cinema()
  {
  playscreen=getmem(PLATNO_S*sizeof(*playscreen));
  memset(playscreen,0xff,PLATNO_S*sizeof(*playscreen));
  playscreen2=getmem(PLATNO_S*sizeof(*playscreen));
  memset(playscreen2,0xff,PLATNO_S*sizeof(*playscreen));
  paleta=getmem(512);
  memset(paleta,0xff,sizeof(paleta));
  if (banking) show_place=(word *)(POZICE*2048);else show_place=lbuffer+POZICE*640;
  curcolor=0;bar(0,0,639,479);
  showview(0,0,0,0);
  decomp_buff=getmem(DECOMP_BUFF);
  }

void done_cinema()
  {
  free(playscreen);
  free(playscreen2);
  free(paleta);
  free(decomp_buff);
  }

void fuse_frames()
  {
  long *p=(long *)playscreen,*q=(long *)playscreen2;
  int i=PLATNO_S/2;
  do
     {
     *q=(*q & 0x7bde7bde)+(*p & 0x7bde7bde)>>1;
     q++;p++;i--;
     }
  while (i);
  }

void copy_frames()
  {
  memcpy(playscreen2,playscreen,PLATNO_S*2);
  }

void display_interlaced(word *playscreen2,int ofs)
  {
  word *p,*q;
  int i=180;
  p=show_place+ofs;
  q=playscreen2;
  do
     {
     if (banking) memcpy(mapvesaadr1(p),q,1280);else memcpy(p,q,1280);
     q+=640;
     p+=banking?2048:1280;i--;
     }
  while (i);
  }

void display_full_lfb(word *playscreen2)
  {
  word *p,*q;
  int i=180;
  p=show_place;
  q=playscreen2;
  do
     {
     int j=320;
     memcpy(p,q,1280);
     p+=640;
     do
        {
        *(long *)p=(*(long *)q & 0x7bde7bde)+(*(long *)(q+640) & 0x7bde7bde)>>1;q+=2;p+=2;j--;
        }
     while(j);
     i--;
     }
  while (i);
  }

void display_full_bank(word *playscreen2)
  {
  word *p,*q;
  long *z;
  int i=180;
  p=show_place;
  q=playscreen2;
  do
     {
     int j=320;
     memcpy(mapvesaadr1(p),q,1280);
     p+=1024;
     z=(long *)mapvesaadr1(p);
     do
        {
        *z=(*(long *)q & 0x7bde7bde)+(*(long *)(q+640) & 0x7bde7bde)>>1;q+=2;z++;j--;
        }
     while(j);
     p+=1024;
     i--;
     }
  while (i);
  }


void show_play_screen()
  {
  if (interlaced)
     {
     if (fusing)
       {
       if (ifuse) display_interlaced(playscreen,(banking?1024:640));
       else
       {
       if (!banking) display_interlaced_fused_lfb(playscreen,playscreen2,show_place);
       else display_interlaced_fused_bank(playscreen,playscreen2,show_place);
       }
       delay(30);
       }
     display_interlaced(playscreen,0);
     }
  else
     {
     if (fusing)
        {
        fuse_frames();
        if (banking) display_full_bank(playscreen2);else display_full_lfb(playscreen2);
        delay(30);
        copy_frames();
        }
     if (banking) display_full_bank(playscreen);else display_full_lfb(playscreen);
     }
  }

void set_direct_mode()
  {
  fusing=0;
  if (!banking)
     {
     direct=1;
     playscreen=show_place;
     anim_line_len=1280*2;
     direct_line_len=640*16;
     }

  }

void play_cinema(FILE *anim)
  {
  char i;

  do
     {
  fread(&i,1,1,anim);
  switch (i)
     {
     case TRACK_VIDEO:
           load_frame(anim);
           decompr_pict();
           break;
     case TRACK_PALETTE:
           load_palette(anim);
           break;
     case TRACK_MUSIC:break;
     case TRACK_END:return;
     case TRACK_DELAY:
           if (!direct)
              show_play_screen();
           break;
     default: return;
     }
  if (_bios_keybrd(_KEYBRD_READY)) return;
     }
  while (1);
  }

void analyze_params(char *c)
  {
  while (*c)
     switch (*c++)
        {
        case 'i':
        case 'I':interlaced=1;break;
        case 's':
        case 'S':fusing=0;break;
        case 'd':
        case 'D':direct=1;break;
        case 'v':
        case 'V':sdiff=1;break;
        case 'f':
        case 'F':ifuse=1;interlaced=1;fusing=1;break;
        }

  }

void main(int argc,char **argv)
  {
  char filename[129];

  if (argc<2)
     {
     puts("Pouziti: PLAYCNM film[.cnm] [I][D][S]");
     puts("     I - Interlaced, prokladane");
     puts("     S  -Skip,vypina dvojnasobny pocet Framu");
     puts("     D - Direct, prima dekomprese do LFB (totez jako IS, ale rychle)");
     puts("     V - View differences, zobrazuje pouze rozdily (jen ty dulezite)");
     puts("     F - Zapina specialni prokladaci rezim + I, ignoruje pri D");
     return;
     }
  if (argc>=3) analyze_params(argv[2]);
  priponu(argv[1],filename,124);
  anim=fopen(filename,"rb");
  if (anim==NULL)
     {
     printf("Nemohu otevrit soubor %s.\n",filename);
     return;
     }
  if (initmode32(NULL))
     {
     puts("Tento program vyzaduje grafickou kartu, ktera podporuje HICOLOR 32768 barev") ;
     return;
     }
  delay(1000);
  init_cinema();
  if (direct) set_direct_mode();
  do
     {
     play_cinema(anim);
     fseek(anim,0,SEEK_SET);
     }
  while (!_bios_keybrd(_KEYBRD_READY));
  done_cinema();
  closemode();
  fclose(anim);
  }
