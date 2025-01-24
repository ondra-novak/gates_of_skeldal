#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <dos.h>
#include "..\types.h"
#include "decoder.c"
#include "..\bgraph.h"


char *gif_buffer,*gif_ptr;
char *decomp_buff,*decomp_ptr;
char *comp_buff,*comp_ptr;

int g_xsize,g_ysize;
char paleta[768];

int get_byte()
  {
  return(*gif_ptr++);
  }

int curline;
char *obrazovka;

int out_line(char *data,int linelen)
  {
  memcpy(obrazovka,data,linelen);
  memcpy(decomp_ptr,data,linelen);
  decomp_ptr+=linelen;
  obrazovka+=640;
  if (++curline>=g_ysize) return (curline);
  return 0;
  }

void setpalette(char *paleta)
  {
  int i;

  for (i=0;i<256;i++)
     {
     outp(0x3c6,0xff);
     outp(0x3c8,i);
     outp(0x3c9,*paleta++>>2);
     outp(0x3c9,*paleta++>>2);
     outp(0x3c9,*paleta++>>2);
     }
  }


void load_gif(char *filename)
  {
  FILE *pic;
  long lengif;


  pic=fopen(filename,"rb");
  if (pic==NULL) return;
  fseek(pic,0,SEEK_END);
  lengif=ftell(pic);
  gif_buffer=(char *)malloc(lengif);gif_ptr=gif_buffer;
  fseek(pic,6,SEEK_SET);
  fread(&g_xsize,2,1,pic);
  fread(&g_ysize,2,1,pic);
  decomp_buff=(char *)malloc(g_xsize*g_ysize);decomp_ptr=decomp_buff;
  fseek (pic, 0x0d, SEEK_SET);
  fread(&paleta,768,1,pic);
  setpalette(paleta);
  fseek (pic, 0x317, SEEK_SET);
  lengif-=0x317;
  fread(gif_buffer,lengif,1,pic);
  curline=0;
  decoder(g_xsize);
  fclose(pic);
  free(gif_buffer);
  }

#define save_nibble(x) if (nibble_sel) {*(comp_ptr++)|=(x)<<4;nibble_sel=!nibble_sel;} else {*(comp_ptr)=(x);nibble_sel=!nibble_sel;}

int Hi_coder_1()
  {
  char palette[8];
  long state[8];
  char nibble_sel=0;
  long picsize,cntr=0;
  long r_size=0;

  memset(palette,0x0,8);
  memset(state,0x0,4*8);
  picsize=g_xsize*g_ysize;

  while (picsize)
     {
     char i,b;

     b=*decomp_ptr++;cntr++;
     for (i=0;i<8;i++) if (palette[i]==b) break;
     if (i!=8)
        {
        save_nibble(i);
        state[i]=cntr;
        r_size++;
        }
        else
        {
        long min=0x7fffffff;char sl=0;

        for (i=0;i<8;i++)
           if (state[i]<min)
           {
           min=state[i];sl=i;
           }
        palette[sl]=b;
        state[sl]=cntr;
        save_nibble(sl+8);
        save_nibble(b & 0xf);
        save_nibble(b >> 4);
        r_size+=3;
        }
     picsize--;
     }
  r_size>>=1;
  return r_size;
  }


void prepare_compress()
  {
  comp_buff=(char *)malloc(512000);
  comp_ptr=comp_buff;
  decomp_ptr=decomp_buff;
  }


int main()
  {
  initmode256("..\xlat256.pal");
  memset(lbuffer,0xff,640*480);
  getchar();
  obrazovka=(char *)lbuffer;
  load_gif("desk_d.gif");
  prepare_compress();
  Hi_coder_1();
  getchar();
  return 0;
  }


